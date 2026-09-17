package gateway

import (
	"context"
	"errors"
	"net"
	"sync"
	"time"
	"unicode/utf8"

	"google.golang.org/protobuf/proto"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/auth"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/persist"
)

type sessionState int

const (
	stHello    sessionState = iota // waiting for Hello
	stAuth                         // waiting for LoginReq
	stLobby                        // logged in, character list / create / enter
	stEntering                     // SessionOpen sent to the zone, waiting for the ack
	stWorld                        // in the zone: world messages are relayed
	stClosed
)

func (s sessionState) String() string {
	return [...]string{"hello", "auth", "lobby", "entering", "world", "closed"}[s]
}

// SessionClose.reason values (internal.proto).
const (
	closeClientLeft uint32 = 0
	closeTimeout    uint32 = 1
	closeKicked     uint32 = 2
	closeShutdown   uint32 = 3
)

// tokenBucket is the per client message rate limit: `rate` frames per second sustained,
// `burst` at once.
type tokenBucket struct {
	rate, burst, tokens float64
	last                time.Time
}

func newBucket(rate float64, burst int, now time.Time) tokenBucket {
	return tokenBucket{rate: rate, burst: float64(burst), tokens: float64(burst), last: now}
}

func (b *tokenBucket) take(now time.Time) bool {
	if now.After(b.last) {
		b.tokens += now.Sub(b.last).Seconds() * b.rate
		if b.tokens > b.burst {
			b.tokens = b.burst
		}
		b.last = now
	}
	if b.tokens < 1 {
		return false
	}
	b.tokens--
	return true
}

type session struct {
	srv    *Server
	sid    uint64
	conn   net.Conn
	remote string
	kind   string // transport the client came in on: tcp, tls, ws, wss
	out    chan []byte
	done   chan struct{}

	mu          sync.Mutex
	state       sessionState
	account     *persist.Account
	playerID    uint64
	role        *jxpb.RoleData
	entityID    uint64
	closeOnce   sync.Once
	closeMsg    string
	kicked      bool
	zoneBound   bool   // SessionOpen sent and not closed yet: the zone must get SessionClose
	leaveReason uint32 // what the zone is told in SessionClose
	loginTries  int
	bucket      tokenBucket
}

func newSession(srv *Server, sid uint64, conn net.Conn) *session {
	return &session{srv: srv, sid: sid, conn: conn, remote: conn.RemoteAddr().String(), kind: "tcp",
		out: make(chan []byte, srv.cfg.OutQueue), done: make(chan struct{}),
		bucket: newBucket(srv.cfg.RateMsgs, srv.cfg.RateBurst, time.Now())}
}

func (s *session) logCtx() context.Context {
	s.mu.Lock()
	c := log.Context{Sid: s.sid, Pid: s.playerID}
	s.mu.Unlock()
	return log.WithContext(context.Background(), c)
}

// readTimeout is how long the client may stay silent in the current state.
func (s *session) readTimeout() time.Duration {
	switch s.getState() {
	case stHello:
		return s.srv.cfg.HelloTimeout
	case stWorld:
		return s.srv.cfg.HeartbeatTimeout
	default:
		return s.srv.cfg.IdleTimeout
	}
}

func (s *session) run() {
	if tc, ok := s.conn.(*net.TCPConn); ok {
		_ = tc.SetNoDelay(true)
	}
	s.srv.stats.Connects.Add(1)
	log.InfoCtx(s.logCtx(), "net", "client connected", log.F("remote", s.remote), log.F("transport", s.kind))
	go s.writer()

	r := frame.NewReader(s.conn, frame.MaxClientPayload)
	for {
		timeout := s.readTimeout()
		_ = s.conn.SetReadDeadline(time.Now().Add(timeout))
		f, err := r.Read()
		if err != nil {
			var ne net.Error
			if errors.As(err, &ne) && ne.Timeout() && s.getState() != stClosed {
				s.srv.stats.Timeouts.Add(1)
				s.setLeaveReason(closeTimeout)
				log.WarnCtx(s.logCtx(), "net", "client timed out", log.F("state", s.getState().String()), log.F("timeout_s", timeout.Seconds()))
			} else if !errors.Is(err, net.ErrClosed) && s.closeMsg == "" {
				log.DebugCtx(s.logCtx(), "net", "client read ended", log.F("error", err))
			}
			break
		}
		if !s.handle(f) {
			break
		}
	}
	s.mu.Lock()
	kicked := s.kicked
	s.mu.Unlock()
	if kicked {
		s.drain(300 * time.Millisecond) // let the Kick frame reach the client before closing
	}
	s.close("read loop ended")
	s.leaveZone()
	if acc := s.getAccount(); acc != nil {
		s.srv.releaseOnline(acc.ID, s)
	}
	s.srv.stats.Disconnects.Add(1)
	log.InfoCtx(s.logCtx(), "net", "client disconnected", log.F("reason", s.closeMsg), log.F("zone_reason", s.getLeaveReason()))
}

func (s *session) writer() {
	for {
		select {
		case b := <-s.out:
			_ = s.conn.SetWriteDeadline(time.Now().Add(s.srv.cfg.WriteTimeout))
			if _, err := s.conn.Write(b); err != nil {
				s.close("write failed: " + err.Error())
				return
			}
			s.srv.stats.FramesOut.Add(1)
			s.srv.stats.BytesOut.Add(uint64(len(b)))
		case <-s.done:
			return
		}
	}
}

// close terminates the connection; safe to call from any goroutine, idempotent.
func (s *session) close(reason string) {
	s.closeOnce.Do(func() {
		s.mu.Lock()
		s.state = stClosed
		s.closeMsg = reason
		s.mu.Unlock()
		close(s.done)
		_ = s.conn.Close()
	})
}

func (s *session) sendRaw(b []byte) {
	select {
	case s.out <- b:
	case <-s.done:
	default:
		s.srv.stats.Dropped.Add(1)
		log.WarnCtx(s.logCtx(), "net", "client too slow, dropping", log.F("queued", len(s.out)))
		s.close("slow consumer")
	}
}

func (s *session) send(id jxpb.MsgId, m proto.Message) {
	payload, err := proto.Marshal(m)
	if err != nil {
		log.ErrorCtx(s.logCtx(), "net", "marshal failed", log.F("msg", int32(id)), log.F("error", err))
		return
	}
	log.TraceCtx(s.logCtx(), "net.send", "to client", log.F("msg", int32(id)), log.F("bytes", len(payload)))
	s.sendRaw(frame.Encode(uint16(id), 0, payload))
}

// kick tells the client why and closes shortly after.  The zone is told "kicked" unless a
// more specific reason (shutdown) was set before.
func (s *session) kick(reason jxpb.Result, text string) {
	s.mu.Lock()
	s.kicked = true
	if s.leaveReason == closeClientLeft {
		s.leaveReason = closeKicked
	}
	s.mu.Unlock()
	s.srv.stats.Kicks.Add(1)
	switch reason {
	case jxpb.Result_RESULT_RATE_LIMITED:
		s.srv.stats.RateKicks.Add(1)
	case jxpb.Result_RESULT_REPLACED:
		s.srv.stats.Replaced.Add(1)
	}
	s.send(jxpb.MsgId_G2C_KICK, &jxpb.Kick{Reason: reason, Text: text})
	log.WarnCtx(s.logCtx(), "net", "kick", log.F("reason", reason.String()), log.F("text", text))
	// the read loop drains the queue and closes; this is the fallback for kicks from elsewhere
	time.AfterFunc(500*time.Millisecond, func() { s.close("kicked: " + text) })
}

// replaced is called when the same account logged in from another connection.
func (s *session) replaced() {
	s.kick(jxpb.Result_RESULT_REPLACED, "account logged in from another client")
}

// shutdown is called by the server when it stops.
func (s *session) shutdown() {
	s.setLeaveReason(closeShutdown)
	s.kick(jxpb.Result_RESULT_SERVER_SHUTDOWN, "server shutting down")
}

// drain waits until queued frames were handed to the socket (bounded).
func (s *session) drain(max time.Duration) {
	deadline := time.Now().Add(max)
	for len(s.out) > 0 && time.Now().Before(deadline) {
		time.Sleep(5 * time.Millisecond)
	}
	time.Sleep(20 * time.Millisecond)
}

func (s *session) getState() sessionState {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.state
}

func (s *session) setState(st sessionState) {
	s.mu.Lock()
	old := s.state
	if old != stClosed {
		s.state = st
	}
	s.mu.Unlock()
	if old != stClosed {
		// the read loop may be blocked with the old state's deadline: apply the new one now
		_ = s.conn.SetReadDeadline(time.Now().Add(s.readTimeout()))
	}
	log.DebugCtx(s.logCtx(), "session", "state", log.F("from", old.String()), log.F("to", st.String()))
}

func (s *session) getAccount() *persist.Account {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.account
}

func (s *session) setLeaveReason(r uint32) {
	s.mu.Lock()
	if s.leaveReason == closeClientLeft {
		s.leaveReason = r
	}
	s.mu.Unlock()
}

func (s *session) getLeaveReason() uint32 {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.leaveReason
}

// handle dispatches one client frame; returns false when the session must end.
func (s *session) handle(f frame.Frame) bool {
	id := jxpb.MsgId(f.MsgID)
	st := s.getState()
	s.srv.stats.FramesIn.Add(1)
	s.srv.stats.BytesIn.Add(uint64(len(f.Payload) + 8))
	log.TraceCtx(s.logCtx(), "net.recv", "from client", log.F("msg", int32(id)), log.F("bytes", len(f.Payload)), log.F("state", st.String()))

	if !s.bucket.take(time.Now()) {
		log.WarnCtx(s.logCtx(), "net", "rate limit exceeded", log.F("msg", int32(id)), log.F("rate", s.srv.cfg.RateMsgs), log.F("burst", s.srv.cfg.RateBurst))
		s.kick(jxpb.Result_RESULT_RATE_LIMITED, "too many messages")
		return false
	}

	if id == jxpb.MsgId_C2G_PING {
		var p jxpb.Ping
		if err := proto.Unmarshal(f.Payload, &p); err != nil {
			return s.bad("Ping")
		}
		s.send(jxpb.MsgId_G2C_PONG, &jxpb.Pong{ClientMs: p.ClientMs, ServerMs: uint64(time.Now().UnixMilli())})
		return true
	}

	switch st {
	case stHello:
		if id != jxpb.MsgId_C2G_HELLO {
			s.kick(jxpb.Result_RESULT_WRONG_STATE, "hello expected")
			return false
		}
		return s.onHello(f)
	case stAuth:
		if id != jxpb.MsgId_C2G_LOGIN {
			s.kick(jxpb.Result_RESULT_WRONG_STATE, "login expected")
			return false
		}
		return s.onLogin(f)
	case stLobby:
		switch id {
		case jxpb.MsgId_C2G_CHAR_LIST:
			return s.onCharList()
		case jxpb.MsgId_C2G_CHAR_CREATE:
			return s.onCharCreate(f)
		case jxpb.MsgId_C2G_ENTER_WORLD:
			return s.onEnterWorld(f)
		case jxpb.MsgId_C2G_LEAVE_WORLD:
			return true
		}
	case stEntering:
		// world traffic before the ack is dropped; the client waits for EnterWorldRes
		return true
	case stWorld:
		switch id {
		case jxpb.MsgId_C2G_MOVE, jxpb.MsgId_C2G_CHAT, jxpb.MsgId_C2G_ATTACK:
			return s.relay(id, f.Payload)
		case jxpb.MsgId_C2G_LEAVE_WORLD:
			s.leaveZone()
			s.setState(stLobby)
			return true
		case jxpb.MsgId_C2G_CHAR_LIST:
			return s.onCharList()
		}
	case stClosed:
		return false
	}
	log.WarnCtx(s.logCtx(), "session", "message not allowed in state", log.F("msg", int32(id)), log.F("state", st.String()))
	return true
}

func (s *session) bad(what string) bool {
	log.WarnCtx(s.logCtx(), "net", "malformed message", log.F("msg", what))
	s.kick(jxpb.Result_RESULT_BAD_REQUEST, "malformed "+what)
	return false
}

func (s *session) onHello(f frame.Frame) bool {
	var h jxpb.Hello
	if err := proto.Unmarshal(f.Payload, &h); err != nil {
		return s.bad("Hello")
	}
	if h.ProtocolVersion != uint32(jxpb.Protocol_PROTOCOL_VERSION) {
		log.WarnCtx(s.logCtx(), "net", "protocol mismatch", log.F("client", h.ProtocolVersion), log.F("server", int32(jxpb.Protocol_PROTOCOL_VERSION)))
		s.kick(jxpb.Result_RESULT_VERSION_MISMATCH, "protocol version mismatch")
		return false
	}
	if s.srv.stopping.Load() {
		s.kick(jxpb.Result_RESULT_SERVER_SHUTDOWN, "server shutting down")
		return false
	}
	log.InfoCtx(s.logCtx(), "net", "hello", log.F("client_version", h.ClientVersion), log.F("platform", h.Platform))
	s.send(jxpb.MsgId_G2C_HELLO_ACK, &jxpb.HelloAck{ProtocolVersion: uint32(jxpb.Protocol_PROTOCOL_VERSION), ServerTimeMs: uint64(time.Now().UnixMilli()),
		ServerVersion: Version, Sid: s.sid, AuthMode: s.srv.auth.Mode(), HeartbeatS: uint32(s.srv.cfg.HeartbeatTimeout.Seconds())})
	s.setState(stAuth)
	return true
}

// loginFailure maps the account server's answer to the client result (old LOGIN_R_* codes).
func loginFailure(err error) (jxpb.Result, string) {
	switch {
	case errors.Is(err, auth.ErrAccountOrPassword):
		return jxpb.Result_RESULT_UNAUTHORIZED, "invalid account or password"
	case errors.Is(err, auth.ErrFrozen):
		return jxpb.Result_RESULT_ACCOUNT_FROZEN, "account frozen"
	case errors.Is(err, auth.ErrNoGameTime):
		return jxpb.Result_RESULT_NO_GAME_TIME, "no game time left"
	case errors.Is(err, auth.ErrBusy):
		return jxpb.Result_RESULT_SERVER_BUSY, "too many failed logins, try again later"
	case errors.Is(err, auth.ErrWeakPassword):
		return jxpb.Result_RESULT_BAD_REQUEST, "password too short"
	default:
		return jxpb.Result_RESULT_INTERNAL_ERROR, "login failed"
	}
}

func (s *session) onLogin(f frame.Frame) bool {
	var req jxpb.LoginReq
	if err := proto.Unmarshal(f.Payload, &req); err != nil {
		return s.bad("LoginReq")
	}
	ctx := context.Background()
	acc, err := s.srv.auth.Login(ctx, req.Account, req.Password, s.remote)
	if err != nil {
		result, text := loginFailure(err)
		if errors.Is(err, auth.ErrFrozen) {
			if a, e := s.srv.store.Account(ctx, req.Account); e == nil && a.FrozenText != "" {
				text = a.FrozenText
			}
		}
		s.loginTries++
		s.srv.stats.LoginFails.Add(1)
		log.WarnCtx(s.logCtx(), "auth", "login failed", log.F("account", req.Account), log.F("result", result.String()), log.F("error", err), log.F("tries", s.loginTries))
		s.send(jxpb.MsgId_G2C_LOGIN_RES, &jxpb.LoginRes{Result: result, Text: text})
		if s.loginTries >= s.srv.cfg.MaxLoginTries {
			s.kick(jxpb.Result_RESULT_SERVER_BUSY, "too many login attempts")
			return false
		}
		return true
	}
	old, ok := s.srv.claimOnline(acc.ID, s, !s.srv.cfg.RefuseDuplicateLogin)
	if !ok {
		log.WarnCtx(s.logCtx(), "auth", "account in use", log.F("account", acc.Name), log.F("account_id", acc.ID), log.F("other_sid", old.sid))
		s.send(jxpb.MsgId_G2C_LOGIN_RES, &jxpb.LoginRes{Result: jxpb.Result_RESULT_ACCOUNT_IN_USE, Text: "account is logged in elsewhere"})
		return true
	}
	if old != nil {
		log.InfoCtx(s.logCtx(), "auth", "replacing session of account", log.F("account", acc.Name), log.F("old_sid", old.sid))
		old.replaced()
	}
	s.mu.Lock()
	s.account = acc
	s.mu.Unlock()
	s.srv.stats.Logins.Add(1)
	log.InfoCtx(s.logCtx(), "auth", "login ok", log.F("account", acc.Name), log.F("account_id", acc.ID), log.F("logins", acc.Logins))
	s.send(jxpb.MsgId_G2C_LOGIN_RES, &jxpb.LoginRes{Result: jxpb.Result_RESULT_OK, AccountId: acc.ID})
	s.setState(stLobby)
	return true
}

func summaryOf(r *jxpb.RoleData) *jxpb.CharSummary {
	zone := uint32(0)
	if r.Position != nil {
		zone = r.Position.ZoneId
	}
	return &jxpb.CharSummary{PlayerId: r.PlayerId, Name: r.Name, Level: r.Level, Series: r.Series, Sex: r.Sex, Faction: r.Faction, ZoneId: zone}
}

func (s *session) onCharList() bool {
	acc := s.getAccount()
	chars, err := s.srv.store.Characters(context.Background(), acc.ID)
	if err != nil {
		log.ErrorCtx(s.logCtx(), "db", "characters failed", log.F("error", err))
		s.send(jxpb.MsgId_G2C_CHAR_LIST_RES, &jxpb.CharListRes{Result: jxpb.Result_RESULT_INTERNAL_ERROR})
		return true
	}
	res := &jxpb.CharListRes{Result: jxpb.Result_RESULT_OK, MaxChars: uint32(s.srv.cfg.MaxChars)}
	for _, c := range chars {
		res.Chars = append(res.Chars, summaryOf(c))
	}
	s.send(jxpb.MsgId_G2C_CHAR_LIST_RES, res)
	return true
}

func (s *session) onCharCreate(f frame.Frame) bool {
	var req jxpb.CharCreateReq
	if err := proto.Unmarshal(f.Payload, &req); err != nil {
		return s.bad("CharCreateReq")
	}
	acc := s.getAccount()
	ctx := context.Background()
	existing, err := s.srv.store.Characters(ctx, acc.ID)
	if err != nil {
		s.send(jxpb.MsgId_G2C_CHAR_CREATE_RES, &jxpb.CharCreateRes{Result: jxpb.Result_RESULT_INTERNAL_ERROR})
		return true
	}
	if len(existing) >= s.srv.cfg.MaxChars {
		s.send(jxpb.MsgId_G2C_CHAR_CREATE_RES, &jxpb.CharCreateRes{Result: jxpb.Result_RESULT_FULL})
		return true
	}
	role, err := s.srv.store.CreateCharacter(ctx, acc.ID, req.Name, req.Series, req.Sex)
	switch {
	case errors.Is(err, persist.ErrInvalidName):
		s.send(jxpb.MsgId_G2C_CHAR_CREATE_RES, &jxpb.CharCreateRes{Result: jxpb.Result_RESULT_INVALID_NAME})
	case errors.Is(err, persist.ErrExists):
		s.send(jxpb.MsgId_G2C_CHAR_CREATE_RES, &jxpb.CharCreateRes{Result: jxpb.Result_RESULT_ALREADY_EXISTS})
	case err != nil:
		log.ErrorCtx(s.logCtx(), "db", "create character failed", log.F("error", err))
		s.send(jxpb.MsgId_G2C_CHAR_CREATE_RES, &jxpb.CharCreateRes{Result: jxpb.Result_RESULT_INTERNAL_ERROR})
	default:
		s.send(jxpb.MsgId_G2C_CHAR_CREATE_RES, &jxpb.CharCreateRes{Result: jxpb.Result_RESULT_OK, Summary: summaryOf(role)})
	}
	return true
}

func (s *session) onEnterWorld(f frame.Frame) bool {
	var req jxpb.EnterWorldReq
	if err := proto.Unmarshal(f.Payload, &req); err != nil {
		return s.bad("EnterWorldReq")
	}
	acc := s.getAccount()
	role, err := s.srv.store.Character(context.Background(), req.PlayerId)
	if err != nil || role.AccountId != acc.ID {
		s.send(jxpb.MsgId_G2C_ENTER_WORLD_RES, &jxpb.EnterWorldRes{Result: jxpb.Result_RESULT_NOT_FOUND})
		return true
	}
	if !s.srv.zone.ready.Load() {
		log.WarnCtx(s.logCtx(), "zone", "enter world while zone unavailable")
		s.send(jxpb.MsgId_G2C_ENTER_WORLD_RES, &jxpb.EnterWorldRes{Result: jxpb.Result_RESULT_ZONE_UNAVAILABLE})
		return true
	}
	s.mu.Lock()
	s.playerID = role.PlayerId
	s.role = role
	s.zoneBound = true
	s.mu.Unlock()
	s.setState(stEntering)
	s.srv.zone.send(jxpb.MsgId_GZ_SESSION_OPEN, &jxpb.SessionOpen{Sid: s.sid, AccountId: acc.ID, Role: role})
	log.InfoCtx(s.logCtx(), "zone", "session open sent", log.F("name", role.Name))
	return true
}

// onZoneAck is called from the zone goroutine with the zone's answer to SessionOpen.
func (s *session) onZoneAck(ack *jxpb.SessionOpenAck) {
	if s.getState() != stEntering {
		return
	}
	if ack.Result != jxpb.Result_RESULT_OK {
		log.WarnCtx(s.logCtx(), "zone", "session open rejected", log.F("result", ack.Result.String()))
		s.mu.Lock()
		s.zoneBound = false
		s.mu.Unlock()
		s.setState(stLobby)
		s.send(jxpb.MsgId_G2C_ENTER_WORLD_RES, &jxpb.EnterWorldRes{Result: ack.Result})
		return
	}
	s.mu.Lock()
	s.entityID = ack.EntityId
	s.mu.Unlock()
	info := s.srv.zone.info()
	mapID, sceneW, sceneH := info.MapId, info.SceneW, info.SceneH
	if ack.MapId != 0 { // a zone with several maps says which one the player landed in
		mapID, sceneW, sceneH = ack.MapId, ack.SceneW, ack.SceneH
	}
	s.setState(stWorld)
	s.send(jxpb.MsgId_G2C_ENTER_WORLD_RES, &jxpb.EnterWorldRes{
		Result: jxpb.Result_RESULT_OK, ZoneId: info.ZoneId, ZoneName: info.ZoneName, EntityId: ack.EntityId, Pos: ack.Pos, TickHz: info.TickHz,
		MapId: mapID, SceneW: sceneW, SceneH: sceneH,
	})
	log.InfoCtx(s.logCtx(), "zone", "entered world", log.F("entity", ack.EntityId), log.F("zone", info.ZoneId))
}

func (s *session) relay(id jxpb.MsgId, payload []byte) bool {
	if id == jxpb.MsgId_C2G_CHAT {
		var c jxpb.ChatReq
		if err := proto.Unmarshal(payload, &c); err != nil || !utf8.ValidString(c.Text) || len(c.Text) > 512 {
			return s.bad("ChatReq")
		}
	}
	if !s.srv.zone.send(jxpb.MsgId_GZ_CLIENT_PACKET, &jxpb.ClientPacket{Sid: s.sid, MsgId: uint32(id), Payload: payload}) {
		log.WarnCtx(s.logCtx(), "zone", "relay dropped: zone unavailable", log.F("msg", int32(id)))
	}
	return true
}

// leaveZone tells the zone the session is gone (once per SessionOpen, whatever way the
// session ended: LeaveWorld, socket closed, kick, timeout, shutdown) with the reason recorded
// so far and notes that a final PlayerSave is owed.
func (s *session) leaveZone() {
	s.mu.Lock()
	inZone := s.zoneBound
	s.zoneBound = false
	reason := s.leaveReason
	s.mu.Unlock()
	if !inZone {
		return
	}
	s.srv.expectSave(s.sid)
	if !s.srv.zone.send(jxpb.MsgId_GZ_SESSION_CLOSE, &jxpb.SessionClose{Sid: s.sid, Reason: reason}) {
		s.srv.saveArrived(s.sid) // link down: nothing will come
		log.WarnCtx(s.logCtx(), "zone", "session close not sent: zone unavailable")
		return
	}
	log.InfoCtx(s.logCtx(), "zone", "session close sent", log.F("reason", reason))
}

// zoneLost is called when the zone link drops: players are sent back to the lobby.
func (s *session) zoneLost() {
	s.mu.Lock()
	inZone := s.state == stWorld || s.state == stEntering
	if inZone {
		s.state = stLobby
	}
	s.zoneBound = false
	s.mu.Unlock()
	if inZone {
		s.send(jxpb.MsgId_G2C_KICK, &jxpb.Kick{Reason: jxpb.Result_RESULT_ZONE_UNAVAILABLE, Text: "zone unavailable"})
	}
}
