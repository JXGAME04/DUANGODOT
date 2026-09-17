// Package gateway is the client facing edge: it owns TCP sessions, authenticates, serves the
// character lobby from persist and relays world traffic between clients and the zone
// (docs/PROTOCOL.md section 3).  Everything a client can do is decided here by session state.
package gateway

import (
	"context"
	"errors"
	"net"
	"sync"
	"sync/atomic"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

const Version = "0.1.0"

type Config struct {
	ID           string        // gateway id sent to the zone
	Listen       string        // client listen address, e.g. ":17100"
	ZoneAddr     string        // "127.0.0.1:17001"
	MaxChars     int           // characters per account
	IdleTimeout  time.Duration // client read timeout (0 = 5 min)
	WriteTimeout time.Duration // 0 = 10 s
	OutQueue     int           // frames buffered per client (0 = 256)
}

func (c *Config) defaults() {
	if c.ID == "" {
		c.ID = "gw1"
	}
	if c.Listen == "" {
		c.Listen = ":17100"
	}
	if c.ZoneAddr == "" {
		c.ZoneAddr = "127.0.0.1:17001"
	}
	if c.MaxChars <= 0 {
		c.MaxChars = 3
	}
	if c.IdleTimeout <= 0 {
		c.IdleTimeout = 5 * time.Minute
	}
	if c.WriteTimeout <= 0 {
		c.WriteTimeout = 10 * time.Second
	}
	if c.OutQueue <= 0 {
		c.OutQueue = 256
	}
}

type Server struct {
	cfg   Config
	store persist.Store
	auth  auth.Authenticator
	zone  *zoneLink

	mu       sync.RWMutex
	sessions map[uint64]*session
	nextSID  atomic.Uint64
	ln       net.Listener
	addr     atomic.Value // string
	wg       sync.WaitGroup
}

func New(cfg Config, store persist.Store, authenticator auth.Authenticator) *Server {
	cfg.defaults()
	s := &Server{cfg: cfg, store: store, auth: authenticator, sessions: map[uint64]*session{}}
	s.zone = newZoneLink(s, cfg.ZoneAddr)
	return s
}

// Addr is the bound client address (valid after Run started listening).
func (s *Server) Addr() string {
	v, _ := s.addr.Load().(string)
	return v
}

// ZoneReady reports whether the zone link completed its handshake.
func (s *Server) ZoneReady() bool { return s.zone.ready.Load() }

// SessionCount returns the number of connected clients.
func (s *Server) SessionCount() int {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return len(s.sessions)
}

// Run listens for clients and keeps the zone link alive until ctx is cancelled.
func (s *Server) Run(ctx context.Context) error {
	ln, err := net.Listen("tcp", s.cfg.Listen)
	if err != nil {
		log.Error("boot", "cannot listen", log.F("addr", s.cfg.Listen), log.F("error", err))
		return err
	}
	s.ln = ln
	s.addr.Store(ln.Addr().String())
	log.Info("boot", "gateway listening", log.F("addr", ln.Addr().String()), log.F("zone", s.cfg.ZoneAddr), log.F("id", s.cfg.ID))

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()
	s.wg.Add(1)
	go func() {
		defer s.wg.Done()
		s.zone.run(ctx)
	}()

	go func() {
		<-ctx.Done()
		_ = ln.Close()
	}()

	for {
		conn, err := ln.Accept()
		if err != nil {
			if ctx.Err() != nil {
				break
			}
			var ne net.Error
			if errors.As(err, &ne) && ne.Timeout() {
				continue
			}
			log.Warn("net", "accept failed", log.F("error", err))
			time.Sleep(50 * time.Millisecond)
			continue
		}
		sid := s.nextSID.Add(1)
		sess := newSession(s, sid, conn)
		s.mu.Lock()
		s.sessions[sid] = sess
		s.mu.Unlock()
		s.wg.Add(1)
		go func() {
			defer s.wg.Done()
			sess.run(ctx)
			s.mu.Lock()
			delete(s.sessions, sid)
			s.mu.Unlock()
		}()
	}

	// shutdown: close every session, then wait
	s.mu.RLock()
	for _, sess := range s.sessions {
		sess.close("gateway shutdown")
	}
	s.mu.RUnlock()
	s.wg.Wait()
	log.Info("boot", "gateway stopped")
	return nil
}

func (s *Server) session(sid uint64) *session {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.sessions[sid]
}

func (s *Server) eachSession(fn func(*session)) {
	s.mu.RLock()
	list := make([]*session, 0, len(s.sessions))
	for _, sess := range s.sessions {
		list = append(list, sess)
	}
	s.mu.RUnlock()
	for _, sess := range list {
		fn(sess)
	}
}
