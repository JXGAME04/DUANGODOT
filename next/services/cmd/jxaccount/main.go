// jxaccount - account administration for the JX NEXT gateway (the account tool of the old
// Sword3PaySys, testAccServer / S3AccServer console).  Works on the gateway's data directory,
// so run it while the gateway is stopped (or let "dev" auth register accounts on first login).
//
//	jxaccount [-data data/gateway] add <account> <password>
//	jxaccount passwd <account> <password>
//	jxaccount freeze <account> [reason]        E_ACCOUNT_FREEZE: the account cannot log in
//	jxaccount unfreeze <account>
//	jxaccount expire <account> <hours>         game time left (0 = unlimited)
//	jxaccount list
//	jxaccount -n 5000 -maps 1,3,7,99 seed      load test data: N accounts + 1 character each, spread over those maps
package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

func usage() {
	fmt.Fprintln(os.Stderr, "usage: jxaccount [-data DIR] [-min-password N] add|passwd|freeze|unfreeze|expire|list|seed ...")
	os.Exit(2)
}

func main() {
	dataDir := flag.String("data", "data/gateway", "gateway data directory (gateway.data_dir)")
	minPassword := flag.Int("min-password", 6, "minimum password length (LOGIN_PASSWORD_MIN_LEN of the old PaySys)")
	seedCount := flag.Int("n", 0, "seed: how many accounts")
	seedFirst := flag.Int("first", 1, "seed: first account number (bot<first> .. bot<first+n-1>)")
	seedPrefix := flag.String("prefix", "bot", "seed: account name prefix (bot1, bot2, ...)")
	seedPassword := flag.String("password", "bot", "seed: the password every seeded account gets")
	seedMaps := flag.String("maps", "", "seed: map ids to spread the characters over, e.g. 1,3,7,99 (empty = the zone's default map)")
	seedZone := flag.Uint("zone", 1, "seed: zone id of the saved position")
	flag.Parse()
	args := flag.Args()
	if len(args) == 0 {
		usage()
	}
	_ = log.Init(log.Options{Level: log.LevelWarn, Console: true, Process: "jxaccount"})
	store, err := persist.OpenFileStore(*dataDir)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer store.Close()
	opt := auth.StrictOptions()
	opt.MinPassword = *minPassword
	accounts := auth.New(store, opt)
	ctx := context.Background()

	need := func(n int) {
		if len(args) < n+1 {
			usage()
		}
	}
	var cmdErr error
	switch args[0] {
	case "add":
		need(2)
		var acc *persist.Account
		if acc, cmdErr = accounts.Register(ctx, args[1], args[2]); cmdErr == nil {
			fmt.Printf("account %s created (id %d)\n", acc.Name, acc.ID)
		}
	case "passwd":
		need(2)
		if cmdErr = accounts.SetPassword(ctx, args[1], args[2]); cmdErr == nil {
			fmt.Printf("password of %s changed\n", args[1])
		}
	case "freeze", "unfreeze":
		need(1)
		cmdErr = update(ctx, store, args[1], func(a *persist.Account) {
			a.Frozen = args[0] == "freeze"
			a.FrozenText = ""
			if a.Frozen {
				a.FrozenText = strings.Join(args[2:], " ")
			}
		})
	case "expire":
		need(2)
		hours, err := strconv.ParseFloat(args[2], 64)
		if err != nil || hours < 0 {
			usage()
		}
		cmdErr = update(ctx, store, args[1], func(a *persist.Account) {
			a.ExpiresAtMs = 0
			if hours > 0 {
				a.ExpiresAtMs = time.Now().Add(time.Duration(hours * float64(time.Hour))).UnixMilli()
			}
		})
	case "list":
		list, err := store.Accounts(ctx)
		if err != nil {
			cmdErr = err
			break
		}
		fmt.Printf("%-6s %-20s %-6s %-8s %-20s %-22s %s\n", "id", "account", "chars", "logins", "last login", "last addr", "state")
		for _, a := range list {
			state := "ok"
			if a.Frozen {
				state = "frozen: " + a.FrozenText
			} else if a.ExpiresAtMs != 0 {
				state = "expires " + time.UnixMilli(a.ExpiresAtMs).Format("2006-01-02 15:04")
			}
			last := "-"
			if a.LastLoginMs != 0 {
				last = time.UnixMilli(a.LastLoginMs).Format("2006-01-02 15:04:05")
			}
			fmt.Printf("%-6d %-20s %-6d %-8d %-20s %-22s %s\n", a.ID, a.Name, len(a.Chars), a.Logins, last, a.LastAddr, state)
		}
	case "seed":
		if *seedCount <= 0 {
			fmt.Fprintln(os.Stderr, "seed needs -n <count>")
			usage()
		}
		ids, err := parseMapList(*seedMaps)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
		cmdErr = seed(ctx, store, accounts, seedOptions{
			count: *seedCount, first: *seedFirst, prefix: *seedPrefix, password: *seedPassword,
			maps: ids, zoneID: uint32(*seedZone),
		})
	default:
		usage()
	}
	if cmdErr != nil {
		switch {
		case errors.Is(cmdErr, persist.ErrExists):
			fmt.Fprintln(os.Stderr, "account already exists")
		case errors.Is(cmdErr, persist.ErrNotFound):
			fmt.Fprintln(os.Stderr, "account not found")
		case errors.Is(cmdErr, persist.ErrInvalidName):
			fmt.Fprintln(os.Stderr, "invalid account name (3-32 letters, digits, _ . -)")
		case errors.Is(cmdErr, auth.ErrWeakPassword):
			fmt.Fprintf(os.Stderr, "password too short (min %d)\n", *minPassword)
		default:
			fmt.Fprintln(os.Stderr, cmdErr)
		}
		os.Exit(1)
	}
}

func update(ctx context.Context, store persist.Store, name string, fn func(*persist.Account)) error {
	acc, err := store.Account(ctx, name)
	if err != nil {
		return err
	}
	fn(acc)
	if err := store.UpdateAccount(ctx, acc); err != nil {
		return err
	}
	fmt.Printf("account %s updated\n", acc.Name)
	return nil
}
