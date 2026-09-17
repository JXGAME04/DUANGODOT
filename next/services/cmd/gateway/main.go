// gateway - JX NEXT client gateway.
//
//	gateway [--config config/gateway.json] [--set key=value]...
//
// Precedence: file < environment (JX_GATEWAY__LISTEN=...) < --set.
package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/internal/gateway"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/config"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

type setFlags []string

func (s *setFlags) String() string     { return strings.Join(*s, ",") }
func (s *setFlags) Set(v string) error { *s = append(*s, v); return nil }

func main() {
	defaultCfg := ""
	if _, err := os.Stat("config/gateway.json"); err == nil {
		defaultCfg = "config/gateway.json"
	}
	cfgPath := flag.String("config", defaultCfg, "JSON config file")
	var sets setFlags
	flag.Var(&sets, "set", "override key=value (repeatable), e.g. -set gateway.listen=:17100")
	flag.Parse()

	cfg := config.New()
	if *cfgPath != "" {
		c, err := config.FromFile(*cfgPath)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(2)
		}
		cfg = c
	}
	cfg.ApplyEnv("JX_")
	for _, kv := range sets {
		k, v, ok := strings.Cut(kv, "=")
		if !ok {
			fmt.Fprintln(os.Stderr, "--set expects key=value")
			os.Exit(2)
		}
		cfg.Set(k, v)
	}

	if err := log.Init(log.Options{
		Level:   log.ParseLevel(cfg.String("log.level", "info")),
		Levels:  cfg.String("log.levels", ""),
		Console: cfg.Bool("log.console", true),
		File:    cfg.String("log.file", ""),
		Process: "gateway",
	}); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(2)
	}
	defer log.Shutdown()
	log.Info("boot", "gateway starting", log.F("version", gateway.Version), log.F("config", *cfgPath))
	log.Info("cfg", "effective config", log.F("json", strings.ReplaceAll(cfg.Dump(), "\n", "")))

	store, err := persist.OpenFileStore(cfg.String("gateway.data_dir", "data/gateway"))
	if err != nil {
		log.Fatal("boot", "cannot open store", log.F("error", err))
		os.Exit(1)
	}
	defer store.Close()

	srv := gateway.New(gateway.Config{
		ID:          cfg.String("gateway.id", "gw1"),
		Listen:      cfg.String("gateway.listen", ":17100"),
		ZoneAddr:    cfg.String("gateway.zone", "127.0.0.1:17001"),
		MaxChars:    int(cfg.Int("gateway.max_chars", 3)),
		IdleTimeout: time.Duration(cfg.Int("gateway.idle_timeout_s", 300)) * time.Second,
	}, store, &auth.Dev{Store: store})

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	go func() {
		<-ctx.Done()
		log.Info("boot", "signal received, shutting down")
	}()
	if err := srv.Run(ctx); err != nil {
		log.Fatal("boot", "gateway failed", log.F("error", err))
		os.Exit(1)
	}
}
