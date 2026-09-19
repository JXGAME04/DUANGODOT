package main

// An optional pprof door, off unless gateway.pprof names an address.
//
// MASTER SPEC 55 and 96: when a load test stops scaling, the answer has to come from a measurement,
// not from reading the code and guessing.  Batching the socket writes halved this gateway's CPU;
// the next guess (the queue's linear scan) changed nothing, which is exactly why a profiler belongs
// in the binary.
//
//	"gateway": { "pprof": "127.0.0.1:19199" }
//	go tool pprof -http : http://127.0.0.1:19199/debug/pprof/profile?seconds=30

import (
	"net/http"
	_ "net/http/pprof" // registers /debug/pprof on the default mux
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

func startProfiler(addr string) {
	if addr == "" {
		return
	}
	srv := &http.Server{Addr: addr, Handler: http.DefaultServeMux, ReadHeaderTimeout: 10 * time.Second}
	go func() {
		log.Warn("boot", "pprof listening: this door exposes the process, keep it on localhost",
			log.F("addr", addr))
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Error("boot", "pprof failed", log.F("error", err))
		}
	}()
}
