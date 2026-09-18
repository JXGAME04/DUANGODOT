package gateway

import (
	"context"
	"sync/atomic"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

// Stats are the counters the gateway keeps about client traffic.  The old cluster had no
// numbers at all: an overloaded Bishop looked exactly like a healthy one until players
// complained.  Here one line per interval (cat=gw.stats) says what the edge is doing, and
// Snapshot() feeds the future metrics endpoint (roadmap 4.1).
type Stats struct {
	Connects    atomic.Uint64
	Disconnects atomic.Uint64
	Logins      atomic.Uint64
	LoginFails  atomic.Uint64
	Kicks       atomic.Uint64
	FramesIn    atomic.Uint64
	FramesOut   atomic.Uint64
	BytesIn     atomic.Uint64
	BytesOut    atomic.Uint64
	Writes      atomic.Uint64 // socket writes: several frames go out in one (see session.writer)
	Dropped     atomic.Uint64 // frames thrown away because a client could not keep up
	RateKicks   atomic.Uint64
	Timeouts    atomic.Uint64
	Replaced    atomic.Uint64
	ZonePackets atomic.Uint64 // messages fanned out from the zone
	ZoneFanout  atomic.Uint64 // client frames those produced
}

// Snapshot is the value of every counter at one moment.
type Snapshot struct {
	Sessions, Online                              int
	Connects, Disconnects, Logins, LoginFails     uint64
	Kicks, RateKicks, Timeouts, Replaced, Dropped uint64
	FramesIn, FramesOut, BytesIn, BytesOut        uint64
	Writes                                        uint64
	ZonePackets, ZoneFanout                       uint64
	Saves, SaveErrors, SaveCoalesced, SaveLost    uint64
	SaveQueue, SavePeak                           int64
	SaveNs                                        uint64
	ZoneReady                                     bool
}

// Snapshot reads every counter (not atomic as a whole, close enough for reporting).
func (s *Server) Snapshot() Snapshot {
	st := &s.stats
	return Snapshot{
		Sessions: s.SessionCount(), Online: s.OnlineCount(), ZoneReady: s.ZoneReady(),
		Connects: st.Connects.Load(), Disconnects: st.Disconnects.Load(), Logins: st.Logins.Load(), LoginFails: st.LoginFails.Load(),
		Kicks: st.Kicks.Load(), RateKicks: st.RateKicks.Load(), Timeouts: st.Timeouts.Load(), Replaced: st.Replaced.Load(), Dropped: st.Dropped.Load(),
		FramesIn: st.FramesIn.Load(), FramesOut: st.FramesOut.Load(), BytesIn: st.BytesIn.Load(), BytesOut: st.BytesOut.Load(),
		Writes:      st.Writes.Load(),
		ZonePackets: st.ZonePackets.Load(), ZoneFanout: st.ZoneFanout.Load(),
		Saves: s.saves.Saves.Load(), SaveErrors: s.saves.Errors.Load(), SaveCoalesced: s.saves.Coalesced.Load(), SaveLost: s.saves.Lost.Load(),
		SaveQueue: int64(s.saves.Len()), SavePeak: s.saves.Peak(), SaveNs: s.saves.SaveNs.Load(),
	}
}

// reportStats logs one line per interval with the rates since the previous one.
func (s *Server) reportStats(ctx context.Context, every time.Duration) {
	t := time.NewTicker(every)
	defer t.Stop()
	prev := s.Snapshot()
	prevAt := time.Now()
	for {
		select {
		case <-ctx.Done():
			return
		case now := <-t.C:
			cur := s.Snapshot()
			secs := now.Sub(prevAt).Seconds()
			if secs <= 0 {
				secs = every.Seconds()
			}
			perSec := func(a, b uint64) uint64 { return uint64(float64(a-b) / secs) }
			log.Info("gw.stats", "gateway stats",
				log.F("sessions", cur.Sessions), log.F("online", cur.Online), log.F("zone_ready", cur.ZoneReady),
				log.F("msg_in_s", perSec(cur.FramesIn, prev.FramesIn)), log.F("msg_out_s", perSec(cur.FramesOut, prev.FramesOut)),
				log.F("kb_in_s", perSec(cur.BytesIn, prev.BytesIn)/1024), log.F("kb_out_s", perSec(cur.BytesOut, prev.BytesOut)/1024),
				log.F("writes_s", perSec(cur.Writes, prev.Writes)),
				log.F("logins", cur.Logins), log.F("login_fails", cur.LoginFails), log.F("kicks", cur.Kicks),
				log.F("rate_kicks", cur.RateKicks), log.F("timeouts", cur.Timeouts), log.F("replaced", cur.Replaced),
				log.F("dropped", cur.Dropped), log.F("zone_fanout", cur.ZoneFanout),
				log.F("saves_s", perSec(cur.Saves, prev.Saves)), log.F("save_queue", cur.SaveQueue), log.F("save_peak", cur.SavePeak),
				log.F("save_ms", saveAvgMs(cur, prev)), log.F("save_errors", cur.SaveErrors), log.F("save_lost", cur.SaveLost))
			prev, prevAt = cur, now
		}
	}
}

// saveAvgMs is the average time of one save since the previous snapshot, in milliseconds.
func saveAvgMs(cur, prev Snapshot) float64 {
	n := cur.Saves + cur.SaveErrors - prev.Saves - prev.SaveErrors
	if n == 0 {
		return 0
	}
	return float64(cur.SaveNs-prev.SaveNs) / float64(n) / 1e6
}
