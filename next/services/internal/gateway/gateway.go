// Package gateway is the client facing edge: it owns TCP sessions, authenticates, serves the
// character lobby from persist and relays world traffic between clients and the zone
// (docs/PROTOCOL.md section 3).  Everything a client can do is decided here by session state.
//
// It is the Bishop of the old server cluster (login, character list, relay to the game
// server) with the protections the old one lacked: one live session per account, heartbeat
// timeouts, a message rate limit per client and a shutdown that waits for the zone to save.
package gateway

import (
	"context"
	"encoding/json"
	"errors"
	"net"
	"sync"
	"sync/atomic"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/transport"
)

const Version = "0.2.0"

type Config struct {
	ID       string // gateway id sent to the zone
	Listen   string // raw TCP address for the PC client and bots, e.g. ":17100" ("" = closed)
	ListenWS string // WebSocket address for web / mobile clients, e.g. ":17102" ("" = closed)
	WSPath   string // WebSocket path ("" = "/ws")
	// TLS for both doors (tcp -> tls, ws -> wss).  Empty = plain, which is what a LAN dev
	// machine uses; a public server must set both.
	CertFile     string
	KeyFile      string
	ZoneAddr     string        // "127.0.0.1:17001"
	MaxChars     int           // characters per account (MAX_PLAYER_PER_ACCOUNT of the old client: 3)
	IdleTimeout  time.Duration // read timeout in the lobby (0 = 5 min)
	WriteTimeout time.Duration // 0 = 10 s
	OutQueue     int           // frames buffered per client (0 = 256)

	// session protection (new in JX NEXT)
	HelloTimeout     time.Duration // a connection that sends no Hello is dropped after this (0 = 10 s)
	HeartbeatTimeout time.Duration // in the world: no frame (the client pings every 5 s) for this long = gone (0 = 30 s)
	RateMsgs         float64       // client frames per second, sustained (0 = 40)
	RateBurst        int           // frames a client may send at once (0 = 100)
	MaxLoginTries    int           // failed logins on one connection before it is kicked (0 = 5)
	// The old PaySys refused a login while the account was online (E_ACCOUNT_EXIST).  The
	// default here is the modern rule: the new login wins and the old session is kicked with
	// RESULT_REPLACED, which also frees accounts left behind by a crashed client.
	RefuseDuplicateLogin bool
	ShutdownWait         time.Duration // how long to wait for the zone's final saves on shutdown (0 = 3 s)
	StatsInterval        time.Duration // one cat=gw.stats line every interval (0 = 30 s, negative = off)
}

func (c *Config) defaults() {
	if c.ID == "" {
		c.ID = "gw1"
	}
	if c.Listen == "" && c.ListenWS == "" {
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
	if c.HelloTimeout <= 0 {
		c.HelloTimeout = 10 * time.Second
	}
	if c.HeartbeatTimeout <= 0 {
		c.HeartbeatTimeout = 30 * time.Second
	}
	if c.RateMsgs <= 0 {
		c.RateMsgs = 40
	}
	if c.RateBurst <= 0 {
		c.RateBurst = 100
	}
	if c.MaxLoginTries <= 0 {
		c.MaxLoginTries = 5
	}
	if c.ShutdownWait <= 0 {
		c.ShutdownWait = 3 * time.Second
	}
	if c.StatsInterval == 0 {
		c.StatsInterval = 30 * time.Second
	}
}

type Server struct {
	cfg   Config
	store persist.Store
	auth  auth.Authenticator
	zone  *zoneLink

	stats Stats

	mu          sync.RWMutex
	sessions    map[uint64]*session
	online      map[uint64]*session // account id -> the one session logged in with it (iClientID of the old PaySys)
	pendingSave map[uint64]struct{} // sids that left the zone and whose final PlayerSave has not arrived yet
	nextSID     atomic.Uint64
	listeners   []transport.Listener
	addr        atomic.Value // string: the raw TCP door
	addrWS      atomic.Value // string: the WebSocket door
	stopping    atomic.Bool
	sessWG      sync.WaitGroup
	zoneWG      sync.WaitGroup
	acceptWG    sync.WaitGroup
}

func New(cfg Config, store persist.Store, authenticator auth.Authenticator) *Server {
	cfg.defaults()
	s := &Server{cfg: cfg, store: store, auth: authenticator, sessions: map[uint64]*session{}, online: map[uint64]*session{}, pendingSave: map[uint64]struct{}{}}
	s.zone = newZoneLink(s, cfg.ZoneAddr)
	return s
}

// Addr is the bound raw TCP address (valid after Run started listening).
func (s *Server) Addr() string {
	v, _ := s.addr.Load().(string)
	return v
}

// AddrWS is the bound WebSocket address, "" when that door is closed.
func (s *Server) AddrWS() string {
	v, _ := s.addrWS.Load().(string)
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

// OnlineCount returns the number of accounts logged in.
func (s *Server) OnlineCount() int {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return len(s.online)
}

// Run opens every configured door (raw TCP, TLS, WebSocket), keeps the zone link alive until
// ctx is cancelled, then kicks every client, tells the zone, waits for the final saves and
// returns.
func (s *Server) Run(ctx context.Context) error {
	listeners, err := transport.Listen(transport.Options{
		TCP: s.cfg.Listen, WS: s.cfg.ListenWS, WSPath: s.cfg.WSPath,
		CertFile: s.cfg.CertFile, KeyFile: s.cfg.KeyFile,
		Status: s.health,
	})
	if err != nil {
		log.Error("boot", "cannot listen", log.F("tcp", s.cfg.Listen), log.F("ws", s.cfg.ListenWS), log.F("error", err))
		return err
	}
	s.listeners = listeners
	for _, l := range listeners {
		switch l.Kind() {
		case "tcp", "tls":
			s.addr.Store(l.Addr().String())
		case "ws", "wss":
			s.addrWS.Store(l.Addr().String())
		}
		log.Info("boot", "gateway listening", log.F("addr", l.Addr().String()), log.F("kind", l.Kind()),
			log.F("zone", s.cfg.ZoneAddr), log.F("id", s.cfg.ID), log.F("auth", s.auth.Mode()),
			log.F("heartbeat_s", s.cfg.HeartbeatTimeout.Seconds()), log.F("rate_msgs", s.cfg.RateMsgs))
	}

	// the zone link outlives ctx: the final saves of the kicked players travel over it
	zctx, zcancel := context.WithCancel(context.Background())
	defer zcancel()
	s.zoneWG.Add(1)
	go func() {
		defer s.zoneWG.Done()
		s.zone.run(zctx)
	}()

	go func() {
		<-ctx.Done()
		for _, l := range listeners {
			_ = l.Close()
		}
	}()
	if s.cfg.StatsInterval > 0 {
		go s.reportStats(ctx, s.cfg.StatsInterval)
	}

	for _, l := range listeners {
		s.acceptWG.Add(1)
		go func(l transport.Listener) {
			defer s.acceptWG.Done()
			s.accept(ctx, l)
		}(l)
	}
	s.acceptWG.Wait()

	s.shutdown()
	zcancel()
	s.zoneWG.Wait()
	log.Info("boot", "gateway stopped")
	return nil
}

// health answers GET /healthz: ready means the gateway can take players right now (the zone
// link is up and it is not shutting down).  tools/dev.py waits for it instead of guessing.
func (s *Server) health() (bool, []byte) {
	snap := s.Snapshot()
	ready := snap.ZoneReady && !s.stopping.Load()
	body, err := json.Marshal(struct {
		Status    string `json:"status"`
		Version   string `json:"version"`
		Gateway   string `json:"gateway"`
		Auth      string `json:"auth_mode"`
		ZoneReady bool   `json:"zone_ready"`
		Sessions  int    `json:"sessions"`
		Online    int    `json:"online"`
		Stopping  bool   `json:"stopping"`
	}{
		Status: map[bool]string{true: "ok", false: "unavailable"}[ready], Version: Version, Gateway: s.cfg.ID,
		Auth: s.auth.Mode(), ZoneReady: snap.ZoneReady, Sessions: snap.Sessions, Online: snap.Online, Stopping: s.stopping.Load(),
	})
	if err != nil {
		return ready, []byte(`{"status":"error"}`)
	}
	return ready, body
}

// accept runs one door until it closes.
func (s *Server) accept(ctx context.Context, l transport.Listener) {
	for {
		conn, err := l.Accept()
		if err != nil {
			if ctx.Err() != nil || errors.Is(err, net.ErrClosed) {
				return
			}
			var ne net.Error
			if errors.As(err, &ne) && ne.Timeout() {
				continue
			}
			log.Warn("net", "accept failed", log.F("kind", l.Kind()), log.F("error", err))
			time.Sleep(50 * time.Millisecond)
			continue
		}
		sid := s.nextSID.Add(1)
		sess := newSession(s, sid, conn)
		sess.kind = l.Kind()
		s.mu.Lock()
		s.sessions[sid] = sess
		s.mu.Unlock()
		s.sessWG.Add(1)
		go func() {
			defer s.sessWG.Done()
			sess.run()
			s.mu.Lock()
			delete(s.sessions, sid)
			s.mu.Unlock()
		}()
	}
}

// shutdown kicks every client (the zone gets SessionClose reason 3 for the ones in the
// world) and waits for the zone's final PlayerSave of each of them, bounded by ShutdownWait.
func (s *Server) shutdown() {
	s.stopping.Store(true)
	var list []*session
	s.mu.RLock()
	for _, sess := range s.sessions {
		list = append(list, sess)
	}
	s.mu.RUnlock()
	log.Info("boot", "shutting down", log.F("sessions", len(list)), log.F("online", s.OnlineCount()))
	for _, sess := range list {
		sess.shutdown()
	}
	deadline := time.Now().Add(s.cfg.ShutdownWait + time.Second)
	done := make(chan struct{})
	go func() {
		s.sessWG.Wait()
		close(done)
	}()
	select {
	case <-done:
	case <-time.After(time.Until(deadline)):
		log.Warn("boot", "sessions still open at shutdown", log.F("count", s.SessionCount()))
	}
	deadline = time.Now().Add(s.cfg.ShutdownWait)
	for time.Now().Before(deadline) {
		if n := s.pendingSaves(); n == 0 {
			break
		}
		time.Sleep(10 * time.Millisecond)
	}
	if n := s.pendingSaves(); n > 0 {
		log.Error("boot", "final saves missing at shutdown", log.F("players", n))
	}
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

// claimOnline records sess as the session of accountID.  When another session holds the
// account it is returned: with replace it has been unbound (the caller kicks it), otherwise
// the claim failed and ok is false.
func (s *Server) claimOnline(accountID uint64, sess *session, replace bool) (old *session, ok bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	old = s.online[accountID]
	if old != nil && old != sess && !replace {
		return old, false
	}
	s.online[accountID] = sess
	if old == sess {
		old = nil
	}
	return old, true
}

// releaseOnline forgets the account of sess unless another session took it over meanwhile.
func (s *Server) releaseOnline(accountID uint64, sess *session) {
	s.mu.Lock()
	if s.online[accountID] == sess {
		delete(s.online, accountID)
	}
	s.mu.Unlock()
}

// expectSave notes that the zone owes a final PlayerSave for sid (after SessionClose).
func (s *Server) expectSave(sid uint64) {
	s.mu.Lock()
	s.pendingSave[sid] = struct{}{}
	s.mu.Unlock()
}

func (s *Server) saveArrived(sid uint64) {
	s.mu.Lock()
	delete(s.pendingSave, sid)
	s.mu.Unlock()
}

func (s *Server) clearPendingSaves() {
	s.mu.Lock()
	s.pendingSave = map[uint64]struct{}{}
	s.mu.Unlock()
}

func (s *Server) pendingSaves() int {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return len(s.pendingSave)
}
