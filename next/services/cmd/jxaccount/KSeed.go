package main

// The `seed` command prepares a load test: N accounts, one character each, spread over a list of
// maps.  A character's map is part of its saved position, and the zone puts an entering player on
// the map its RoleData names (KGameServer::handle_session_open), so this is all it takes to test
// a zone with players on many maps at once.  No server switch, no protocol addition: the
// distribution is test data.
//
// Run it with the gateway stopped, like the other jxaccount commands: both write the same files.

import (
	"context"
	"fmt"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

type seedOptions struct {
	count    int
	first    int // first account number, so two stores can hold two halves of one population
	prefix   string
	password string
	maps     []uint32
	zoneID   uint32
}

// parseMapList turns "1,3,7,99" into map ids; "" means the zone's default map only.
func parseMapList(s string) ([]uint32, error) {
	if strings.TrimSpace(s) == "" {
		return []uint32{0}, nil
	}
	var out []uint32
	for _, part := range strings.Split(s, ",") {
		part = strings.TrimSpace(part)
		if part == "" {
			continue
		}
		id, err := strconv.ParseUint(part, 10, 32)
		if err != nil {
			return nil, fmt.Errorf("map list: %q is not a map id", part)
		}
		out = append(out, uint32(id))
	}
	if len(out) == 0 {
		return nil, fmt.Errorf("map list is empty")
	}
	return out, nil
}

// seed creates the accounts and characters that are missing and leaves the rest alone, so running
// it again after a bigger -n only adds the new ones.
func seed(ctx context.Context, store persist.Store, accounts *auth.S3PAccount, opt seedOptions) error {
	var created, existed, failed atomic.Int64
	start := time.Now()

	// Hashing a password costs ~16 ms of CPU on purpose, so seeding 5000 accounts is the one place
	// that deserves every core.
	workers := runtime.NumCPU()
	if workers > opt.count {
		workers = opt.count
	}
	jobs := make(chan int)
	var wg sync.WaitGroup
	var firstErr error
	var errOnce sync.Once

	for w := 0; w < workers; w++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := range jobs {
				name := fmt.Sprintf("%s%d", opt.prefix, opt.first+i)
				err := seedOne(ctx, store, accounts, opt, name, opt.maps[i%len(opt.maps)])
				switch {
				case err == nil:
					created.Add(1)
				case err == errAlreadySeeded:
					existed.Add(1)
				default:
					failed.Add(1)
					errOnce.Do(func() { firstErr = fmt.Errorf("%s: %w", name, err) })
				}
			}
		}()
	}
	for i := 0; i < opt.count; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()

	perMap := map[uint32]int{}
	for i := 0; i < opt.count; i++ {
		perMap[opt.maps[i%len(opt.maps)]]++
	}
	fmt.Printf("seed: %d created, %d already there, %d failed, %d maps, %s\n",
		created.Load(), existed.Load(), failed.Load(), len(opt.maps), time.Since(start).Round(time.Millisecond))
	if len(opt.maps) > 1 {
		fmt.Printf("  %d..%d characters per map\n", minCount(perMap), maxCount(perMap))
	}
	if failed.Load() > 0 {
		return firstErr
	}
	return nil
}

// errAlreadySeeded marks an account that already has its character: nothing to do.
var errAlreadySeeded = fmt.Errorf("already seeded")

func seedOne(ctx context.Context, store persist.Store, accounts *auth.S3PAccount, opt seedOptions, name string, mapID uint32) error {
	acc, err := store.Account(ctx, name)
	if err != nil {
		if acc, err = accounts.Register(ctx, name, opt.password); err != nil {
			return err
		}
	} else if err := accounts.SetPassword(ctx, name, opt.password); err != nil {
		// An account left over from an older run keeps its old password, and the whole run then
		// fails on "wrong password".  Seeding owns these names, so it sets the password every time.
		return err
	}
	chars, err := store.Characters(ctx, acc.ID)
	if err != nil {
		return err
	}
	if len(chars) > 0 {
		// Already has a character: make sure it is on the map this run wants it on.
		role, err := store.Character(ctx, chars[0].PlayerId)
		if err != nil {
			return err
		}
		if role.Position == nil || role.Position.MapId != mapID {
			role.Position = &jxpb.RolePosition{ZoneId: opt.zoneID, MapId: mapID}
			if err := store.SaveCharacter(ctx, role); err != nil {
				return err
			}
		}
		return errAlreadySeeded
	}
	role, err := store.CreateCharacter(ctx, acc.ID, persist.NewCharacter{Name: characterName(name)})
	if err != nil {
		return err
	}
	// Only the map, never a position: with no pos the zone starts the player at that map's own
	// spawn point (KSubWorld::spawn_player) instead of at a corner that may not be walkable.
	role.Position = &jxpb.RolePosition{ZoneId: opt.zoneID, MapId: mapID}
	return store.SaveCharacter(ctx, role)
}

// characterName keeps the account name but makes it look like a name a player would pick, and
// character names are unique across the whole store.
func characterName(account string) string {
	if account == "" {
		return account
	}
	return strings.ToUpper(account[:1]) + account[1:]
}

func minCount(m map[uint32]int) int {
	out := -1
	for _, v := range m {
		if out < 0 || v < out {
			out = v
		}
	}
	return out
}

func maxCount(m map[uint32]int) int {
	out := 0
	for _, v := range m {
		if v > out {
			out = v
		}
	}
	return out
}
