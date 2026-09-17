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

	// the account server: "dev" registers any new name on first login, "strict" only knows
	// accounts made with jxaccount
	var opt auth.Options
	switch mode := cfg.String("gateway.auth_mode", "dev"); mode {
	case "dev":
		opt = auth.DevOptions()
	case "strict":
		opt = auth.StrictOptions()
	default:
		log.Fatal("boot", "gateway.auth_mode must be dev or strict", log.F("value", mode))
		os.Exit(2)
	}
	if v := cfg.Int("gateway.min_password", 0); v > 0 {
		opt.MinPassword = int(v)
	}
	if v := cfg.Int("gateway.max_fails", 0); v > 0 {
		opt.MaxFails = int(v)
	}
	if v := cfg.Int("gateway.lock_s", 0); v > 0 {
		opt.LockFor = time.Duration(v) * time.Second
	}
	accounts := auth.New(store, opt)

	srv := gateway.New(gateway.Config{
		ID:                   cfg.String("gateway.id", "gw1"),
		Listen:               cfg.String("gateway.listen", ":17100"),
		ListenWS:             cfg.String("gateway.listen_ws", ""),
		WSPath:               cfg.String("gateway.ws_path", "/ws"),
		CertFile:             cfg.String("gateway.tls_cert", ""),
		KeyFile:              cfg.String("gateway.tls_key", ""),
		ZoneAddr:             cfg.String("gateway.zone", "127.0.0.1:17001"),
		MaxChars:             int(cfg.Int("gateway.max_chars", 3)),
		IdleTimeout:          time.Duration(cfg.Int("gateway.idle_timeout_s", 300)) * time.Second,
		HelloTimeout:         time.Duration(cfg.Int("gateway.hello_timeout_s", 10)) * time.Second,
		HeartbeatTimeout:     time.Duration(cfg.Int("gateway.heartbeat_timeout_s", 30)) * time.Second,
		RateMsgs:             float64(cfg.Int("gateway.rate_msgs", 40)),
		RateBurst:            int(cfg.Int("gateway.rate_burst", 100)),
		MaxLoginTries:        int(cfg.Int("gateway.max_login_tries", 5)),
		RefuseDuplicateLogin: cfg.Bool("gateway.refuse_duplicate_login", false),
		ShutdownWait:         time.Duration(cfg.Int("gateway.shutdown_wait_s", 3)) * time.Second,
		StatsInterval:        time.Duration(cfg.Int("gateway.stats_interval_s", 30)) * time.Second,
	}, store, accounts)

	startProfiler(cfg.String("gateway.pprof", ""))

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
