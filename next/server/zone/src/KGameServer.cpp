#include "jx/zone/KGameServer.h"

#include <algorithm>
#include <chrono>

#include <fmt/format.h>

#include "jx/core/Metrics.h"
#include "jx/internal.pb.h"
#include "jx/log.hpp"
#include "jx/process.hpp"
#include "jx/msg.pb.h"
#include "jx/net/proto.hpp"

namespace jx::zone {

using namespace std::chrono_literals;

namespace {

// How many simulation workers to start when the configuration says "decide for me" (SPEC 15):
// leave a couple of threads for the network and the operating system, never more workers than
// there are map instances to tick.
unsigned auto_workers(std::size_t instances)
{
    const unsigned hw = core::ThreadPool::hardware_threads();
    unsigned n = hw > 3 ? hw - 2 : 1;
    n = std::min<unsigned>(n, static_cast<unsigned>(std::max<std::size_t>(instances, 1)));
    return std::max(1u, n);
}

} // namespace

KGameServer::KGameServer(asio::io_context& io, KGameServerConfig cfg)
    : io_(io), cfg_(std::move(cfg)), listener_(io, frame::kMaxInternalPayload),
      scheduler_(1), clock_(core::TickRate{cfg_.world.tick_hz}),
      fixed_(clock_, cfg_.max_ticks_per_update), profile_(core::metrics(), "zone"), timer_(io)
{
    // KSubWorldSet: one instance per hosted map; the first is the default one
    if (cfg_.worlds.empty()) cfg_.worlds.push_back(cfg_.world);
    std::uint32_t instance_id = 1;
    for (const KSubWorldConfig& wc : cfg_.worlds) {
        auto inst = std::make_unique<KMapInstance>(instance_id, wc);
        if (instance_by_map_.contains(inst->map_id())) {
            log::warn("boot", "map hosted twice, second copy ignored", {log::kv("map", inst->map_id())});
            continue;
        }
        instance_by_map_[inst->map_id()] = inst.get();
        instance_ptrs_.push_back(inst.get());
        instances_.push_back(std::move(inst));
        ++instance_id;
    }
    workers_ = cfg_.simulation_threads != 0 ? cfg_.simulation_threads : auto_workers(instances_.size());
    scheduler_ = KWorldScheduler(workers_);
    scheduler_.assign_all(instance_ptrs_);
    rebuild_buckets();
}

KGameServer::~KGameServer()
{
    if (pool_) pool_->stop();
}

KMapInstance* KGameServer::instance_of_map(std::uint32_t map_id) noexcept
{
    const auto it = instance_by_map_.find(map_id);
    return it == instance_by_map_.end() ? nullptr : it->second;
}

KSubWorld* KGameServer::world_of_map(std::uint32_t map_id) noexcept
{
    KMapInstance* inst = instance_of_map(map_id);
    return inst == nullptr ? nullptr : &inst->world();
}

KMapInstance* KGameServer::instance_of_session(std::uint64_t sid) noexcept
{
    const auto it = session_instance_.find(sid);
    return it == session_instance_.end() ? nullptr : it->second;
}

void KGameServer::bind_session(std::uint64_t sid, KMapInstance& target, std::uint64_t conn_id)
{
    session_instance_[sid] = &target;
    session_gateway_[sid] = conn_id;
}

void KGameServer::unbind_session(std::uint64_t sid)
{
    session_instance_.erase(sid);
    session_gateway_.erase(sid);
}

std::size_t KGameServer::total_entities() const noexcept
{
    std::size_t n = 0;
    for (const auto& inst : instances_) n += inst->entities();
    return n;
}

std::vector<KWorkerLoad> KGameServer::worker_loads() const
{
    std::vector<KMapInstance*> list(instance_ptrs_);
    return scheduler_.loads(list);
}

void KGameServer::rebuild_buckets()
{
    buckets_.assign(workers_, {});
    for (KMapInstance* m : instance_ptrs_) {
        const unsigned w = m->owner() < workers_ ? m->owner() : 0u;
        buckets_[w].push_back(m);
    }
}

std::error_code KGameServer::start()
{
    if (const auto ec = listener_.open(cfg_.listen_address, cfg_.port)) {
        log::error("boot", "cannot listen", {log::kv("addr", cfg_.listen_address), log::kv("port", cfg_.port), log::kv("error", ec.message())});
        return ec;
    }
    // the simulation pool: the only threads that ever touch world state (SPEC 2, 14)
    core::ThreadPoolConfig pc;
    pc.name = "sim";
    pc.threads = workers_;
    pool_ = std::make_unique<core::ThreadPool>(pc);
    jobs_ = std::make_unique<core::JobSystem>(*pool_);

    listener_.start([this](net::Connection::Ptr c) { on_accept(std::move(c)); });
    running_ = true;
    last_update_ = steady_now();
    schedule_tick();
    log::info("boot", "zone listening", {log::kv("addr", cfg_.listen_address), log::kv("port", listener_.port()),
                                         log::kv("zone", cfg_.world.zone_id), log::kv("name", cfg_.world.name),
                                         log::kv("tick_hz", cfg_.world.tick_hz), log::kv("maps", instances_.size()),
                                         log::kv("sim_workers", workers_), log::kv("hardware_threads", core::ThreadPool::hardware_threads())});
    return {};
}

void KGameServer::stop()
{
    if (!running_) return;
    running_ = false;
    timer_.cancel();
    listener_.stop();
    // close() reports on_close synchronously, which erases from gateways_: never iterate the map itself
    std::vector<net::Connection::Ptr> links;
    for (auto& [id, gw] : gateways_) {
        if (gw.conn) links.push_back(gw.conn);
    }
    for (auto& conn : links) conn->close();
    if (pool_) pool_->stop();   // SPEC 87: every thread is joined, never terminated
    log::info("boot", "zone stopped", {log::kv("tick", clock_.tick()), log::kv("players", session_instance_.size())});
}

// ---- connections ---------------------------------------------------------------------------

void KGameServer::on_accept(net::Connection::Ptr conn)
{
    const std::uint64_t id = conn->id();
    gateways_[id] = Gateway{conn, "", false};
    log::info("net", "gateway connected", {log::kv("conn", id), log::kv("remote", conn->remote())});
    conn->start([this](net::Connection& c, const frame::View& v) { on_frame(c, v); },
                [this](net::Connection& c, const std::error_code& ec) { on_close(c, ec); });
}

void KGameServer::on_close(net::Connection& conn, const std::error_code& ec)
{
    const auto it = gateways_.find(conn.id());
    if (it == gateways_.end()) return;
    std::size_t dropped = 0;
    for (auto sit = session_gateway_.begin(); sit != session_gateway_.end();) {
        if (sit->second == conn.id()) {
            const std::uint64_t sid = sit->first;
            if (KMapInstance* inst = instance_of_session(sid)) {
                inst->post(KCmdRemovePlayer{sid, false});   // the gateway is gone: nobody to save to
            }
            session_instance_.erase(sid);
            sit = session_gateway_.erase(sit);
            ++dropped;
        } else {
            ++sit;
        }
    }
    // a link that never completed the handshake is usually a port probe (tools/dev.py), not a gateway
    log::write(it->second.ready ? log::Level::warn : log::Level::debug, "net", "gateway disconnected",
               {log::kv("conn", conn.id()), log::kv("gateway", it->second.id), log::kv("players_dropped", dropped),
                log::kv("error", ec ? ec.message() : "")});
    gateways_.erase(it);
}

void KGameServer::on_frame(net::Connection& conn, const frame::View& view)
{
    const auto it = gateways_.find(conn.id());
    if (it == gateways_.end()) return;
    Gateway& gw = it->second;
    if (!gw.ready && view.msg_id != pb::GZ_ZONE_HELLO) {
        log::warn("net", "frame before hello", {log::kv("conn", conn.id()), log::kv("msg", view.msg_id)});
        conn.close();
        return;
    }
    switch (view.msg_id) {
    case pb::GZ_ZONE_HELLO: handle_hello(gw, view); break;
    case pb::GZ_SESSION_OPEN: handle_session_open(gw, view); break;
    case pb::GZ_SESSION_CLOSE: handle_session_close(gw, view); break;
    case pb::GZ_CLIENT_PACKET: handle_client_packet(gw, view); break;
    default:
        log::warn("net", "unknown internal message", {log::kv("conn", conn.id()), log::kv("msg", view.msg_id)});
        break;
    }
}

void KGameServer::handle_hello(Gateway& gw, const frame::View& view)
{
    pb::ZoneHello hello;
    if (!net::parse(view, hello)) {
        log::warn("net", "bad ZoneHello", {log::kv("conn", gw.conn->id())});
        gw.conn->close();
        return;
    }
    if (hello.protocol_version() != pb::PROTOCOL_VERSION) {
        log::error("net", "gateway protocol mismatch", {log::kv("conn", gw.conn->id()), log::kv("theirs", hello.protocol_version()),
                                                         log::kv("ours", static_cast<int>(pb::PROTOCOL_VERSION))});
        gw.conn->close();
        return;
    }
    gw.id = hello.gateway_id();
    gw.ready = true;
    pb::ZoneHelloAck ack;
    ack.set_protocol_version(pb::PROTOCOL_VERSION);
    ack.set_zone_id(cfg_.world.zone_id);
    ack.set_zone_name(cfg_.world.name);
    ack.set_tick_hz(cfg_.world.tick_hz);
    ack.set_capacity(cfg_.world.max_players);
    ack.set_map_id(world().map_id());
    ack.set_scene_w(static_cast<std::uint32_t>(world().config().width));
    ack.set_scene_h(static_cast<std::uint32_t>(world().config().height));
    // Every gateway numbers its sessions from 1.  With more than one in front of this zone their
    // ids collide and the second gateway's session 1 quietly replaces the first one's: measured
    // with two gateways and 10 000 clients, half of them never reached the world.  Each link gets
    // its own high 16 bits here, so a session id is unique across the whole zone.
    const std::uint64_t prefix = (gw.conn->id() & 0xffffULL) << 48;
    ack.set_session_prefix(prefix);
    net::send_urgent(*gw.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_HELLO_ACK), ack);
    log::info("net", "gateway ready", {log::kv("conn", gw.conn->id()), log::kv("gateway", gw.id),
                                       log::kv("session_prefix", prefix)});
}

// The network thread validates and forwards; the world answers on its own tick (SPEC 30).
void KGameServer::handle_session_open(Gateway& gw, const frame::View& view)
{
    pb::SessionOpen open;
    if (!net::parse(view, open)) {
        log::warn("net", "bad SessionOpen", {log::kv("conn", gw.conn->id())});
        return;
    }
    // the saved map when this zone hosts it (g_SubWorldSet.SearchWorld), else the default map
    KMapInstance* target = open.role().has_position() ? instance_of_map(open.role().position().map_id()) : nullptr;
    if (target == nullptr) {
        target = instances_.front().get();
        if (open.role().has_position() && open.role().position().map_id() != 0) {
            log::warn("zone", "saved map not hosted", {log::kv("sid", open.sid()), log::kv("map", open.role().position().map_id()), log::kv("instead", target->map_id())});
        }
    }
    bind_session(open.sid(), *target, gw.conn->id());
    KCmdSpawnPlayer cmd;
    cmd.sid = open.sid();
    cmd.conn_id = gw.conn->id();
    cmd.role = open.role();
    cmd.account = open.account();
    cmd.reason = KSpawnReason::enter_world;
    target->post(std::move(cmd));
    log::debug("zone", "session open queued", {log::kv("sid", open.sid()), log::kv("map", target->map_id())});
}

void KGameServer::handle_session_close(Gateway& gw, const frame::View& view)
{
    pb::SessionClose close;
    if (!net::parse(view, close)) {
        log::warn("net", "bad SessionClose", {log::kv("conn", gw.conn->id())});
        return;
    }
    const auto it = session_gateway_.find(close.sid());
    if (it == session_gateway_.end() || it->second != gw.conn->id()) {
        log::debug("zone", "close for unknown session", {log::kv("sid", close.sid())});
        return;
    }
    // reason: 0 client left, 1 heartbeat timeout, 2 kicked / replaced by a new login, 3 gateway shutdown
    log::info("zone", "session closed", {log::kv("sid", close.sid()), log::kv("reason", close.reason())});
    if (KMapInstance* inst = instance_of_session(close.sid())) {
        inst->post(KCmdRemovePlayer{close.sid(), true});   // the final PlayerSave comes back as an event
    }
    session_instance_.erase(close.sid());
    // the gateway binding stays until the save event has been sent
}

void KGameServer::handle_client_packet(Gateway& gw, const frame::View& view)
{
    pb::ClientPacket cp;
    if (!net::parse(view, cp)) {
        log::warn("net", "bad ClientPacket", {log::kv("conn", gw.conn->id())});
        return;
    }
    const auto it = session_gateway_.find(cp.sid());
    if (it == session_gateway_.end() || it->second != gw.conn->id()) {
        log::debug("zone", "packet for unknown session", {log::kv("sid", cp.sid()), log::kv("msg", cp.msg_id())});
        return;
    }
    KMapInstance* inst = instance_of_session(cp.sid());
    if (inst == nullptr) return;
    switch (cp.msg_id()) {
    case pb::C2G_MOVE:
    case pb::C2G_CHAT:
    case pb::C2G_ATTACK:
    case pb::C2G_ITEM_MOVE:
    case pb::C2G_ITEM_EQUIP:
    case pb::C2G_ITEM_UNEQUIP:
    case pb::C2G_ITEM_USE:
    case pb::C2G_ITEM_DROP:
    case pb::C2G_PICK_UP:
    case pb::C2G_ADD_POINT:
    case pb::C2G_ADD_SKILL_POINT:
    case pb::C2G_CAST_SKILL:
    case pb::C2G_REVIVE:
    case pb::C2G_RIDE:
    case pb::C2G_SIT:
    case pb::C2G_PK_STATE:
    case pb::C2G_TRADE:
    case pb::C2G_TEAM:
    case pb::C2G_NPC_DIALOG:
    case pb::C2G_DIALOG_ANSWER:
    case pb::C2G_TASK_VALUE:
    case pb::C2G_SKILL_DESC:
    case pb::C2G_SET_AURA: {
        KCmdClientPacket cmd;
        cmd.sid = cp.sid();
        cmd.msg_id = cp.msg_id();
        cmd.payload = cp.payload();
        inst->post(std::move(cmd));
        break;
    }
    default:
        log::warn("zone", "unhandled client message", {log::kv("sid", cp.sid()), log::kv("msg", cp.msg_id())});
        break;
    }
}

// ---- output --------------------------------------------------------------------------------

void KGameServer::flush_outbox()
{
    for (auto& inst : instances_) {
        for (Packet& p : inst->take_outbox()) {
            // group the sessions of one packet by the gateway that owns them
            std::unordered_map<std::uint64_t, pb::ZonePacket> per_gateway;
            for (const std::uint64_t sid : p.sids) {
                const auto it = session_gateway_.find(sid);
                if (it == session_gateway_.end()) continue;
                per_gateway[it->second].add_sids(sid);
            }
            const bool movement = p.msg_id == static_cast<std::uint16_t>(pb::G2C_ENTITY_MOVE) ||
                                  p.msg_id == static_cast<std::uint16_t>(pb::G2C_ENTITY_MOVES);   // the far batches too (N3)
            for (auto& [conn_id, zp] : per_gateway) {
                const auto git = gateways_.find(conn_id);
                if (git == gateways_.end() || !git->second.conn) continue;
                // MASTER SPEC 69/70 on the zone side of the link: when a gateway is already this
                // far behind, a position from this tick would arrive after the next one anyway,
                // so it is dropped instead of growing the backlog.  Nothing else is ever dropped.
                if (movement && git->second.conn->queued_bytes() > kLinkBacklogLimit) {
                    ++link_dropped_;
                    continue;
                }
                zp.set_msg_id(p.msg_id);
                zp.set_payload(p.payload);
                net::send(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_PACKET), zp);
            }
        }
    }
}

// the relay's broadcast classes done here: WORLD and CITY reach every session of the zone, a faction line the
// sessions whose character is of that faction (the map instances are idle between the tick and the flush, so their
// worlds may be read).  One ZonePacket per gateway carries all of its sessions.
void KGameServer::send_chat(const KEvChat& e)
{
    std::unordered_map<std::uint64_t, pb::ZonePacket> per_gateway;
    std::size_t receivers = 0;
    for (const auto& [sid, inst] : session_instance_) {
        if (inst == nullptr) continue;
        if (e.channel == pb::CH_FACTION) {
            const KNpc* p = inst->world().find_player(sid);
            if (p == nullptr || p->player.faction.current != e.faction) continue;
        }
        const auto git = session_gateway_.find(sid);
        if (git == session_gateway_.end()) continue;
        per_gateway[git->second].add_sids(sid);
        ++receivers;
    }
    for (auto& [conn_id, zp] : per_gateway) {
        const auto git = gateways_.find(conn_id);
        if (git == gateways_.end() || !git->second.conn) continue;
        zp.set_msg_id(static_cast<std::uint32_t>(pb::G2C_CHAT_MSG));
        zp.set_payload(e.payload);
        net::send(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_PACKET), zp);
    }
    log::debug("zone.chat", "chat spread", {log::kv("channel", static_cast<int>(e.channel)), log::kv("faction", e.faction), log::kv("receivers", receivers)});
}

void KGameServer::send_to_session(std::uint64_t sid, std::uint16_t msg_id, const google::protobuf::MessageLite& msg)
{
    const auto it = session_gateway_.find(sid);
    if (it == session_gateway_.end()) return;
    const auto git = gateways_.find(it->second);
    if (git == gateways_.end() || !git->second.conn) return;
    pb::ZonePacket zp;
    zp.add_sids(sid);
    zp.set_msg_id(msg_id);
    zp.set_payload(msg.SerializeAsString());
    net::send(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_PACKET), zp);
}

void KGameServer::send_save(const KEvPlayerSave& save)
{
    const auto it = session_gateway_.find(save.sid);
    if (it == session_gateway_.end()) return;
    const auto git = gateways_.find(it->second);
    if (git == gateways_.end() || !git->second.conn) return;
    pb::PlayerSave msg;
    msg.set_sid(save.sid);
    msg.set_final(save.final);
    msg.set_tick(save.tick);
    *msg.mutable_role() = save.role;
    net::send_urgent(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_PLAYER_SAVE), msg);
    if (save.final) session_gateway_.erase(save.sid);   // the session is finished with this zone
}

// Events are handled on the server thread, between the parallel tick and the packet flush, so
// an ack or a ChangeMap always reaches the client before the spawn packets of that same tick.
void KGameServer::process_events()
{
    for (auto& inst : instances_) {
        for (KWorldEvent& ev : inst->take_events()) handle_event(*inst, ev);
    }
}

void KGameServer::handle_event(KMapInstance& source, KWorldEvent& ev)
{
    std::visit(overloaded{
                   [&](KEvSessionOpened& e) {
                       if (e.reason == KSpawnReason::enter_world) {
                           pb::SessionOpenAck ack;
                           ack.set_sid(e.sid);
                           ack.set_result(e.result);
                           if (e.result == pb::RESULT_OK) {
                               ack.set_map_id(e.map_id);
                               ack.set_scene_w(e.scene_w);
                               ack.set_scene_h(e.scene_h);
                               ack.set_entity_id(e.entity.value);
                               ack.mutable_pos()->set_x(e.pos.x);
                               ack.mutable_pos()->set_y(e.pos.y);
                           } else {
                               log::warn("zone", "session open rejected", {log::kv("sid", e.sid), log::kv("result", static_cast<int>(e.result))});
                               unbind_session(e.sid);
                           }
                           const auto git = gateways_.find(e.conn_id);
                           if (git != gateways_.end() && git->second.conn) {
                               net::send_urgent(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_SESSION_OPEN_ACK), ack);
                           }
                       } else {   // arrived through a trap: the client must load the new bundle first
                           if (e.result != pb::RESULT_OK) {
                               log::error("zone.trap", "spawn in the new map failed", {log::kv("sid", e.sid), log::kv("map", e.map_id),
                                                                                      log::kv("result", static_cast<int>(e.result))});
                               unbind_session(e.sid);
                               return;
                           }
                           pb::ChangeMap change;
                           change.set_map_id(e.map_id);
                           change.mutable_pos()->set_x(e.pos.x);
                           change.mutable_pos()->set_y(e.pos.y);
                           change.set_scene_w(e.scene_w);
                           change.set_scene_h(e.scene_h);
                           change.set_entity_id(e.entity.value);
                           send_to_session(e.sid, static_cast<std::uint16_t>(pb::G2C_CHANGE_MAP), change);
                           log::info("zone.trap", "player changed map", {log::kv("sid", e.sid), log::kv("to", e.map_id),
                                                                        log::kv("x", e.pos.x), log::kv("y", e.pos.y), log::kv("entity", e.entity)});
                       }
                   },
                   [&](KEvPlayerSave& e) { send_save(e); },
                   [&](KEvChat& e) { send_chat(e); },
                   [&](KEvWorldChange& e) {
                       KMapInstance* target = instance_of_map(e.map_id);
                       if (target == nullptr) {   // 不在这台服务器上: TobeExchangeServer is not there yet
                           log::warn("zone.trap", "map not hosted here, player goes back", {log::kv("sid", e.sid), log::kv("map", e.map_id)});
                           KCmdSpawnPlayer back;
                           back.sid = e.sid;
                           back.conn_id = session_gateway_.count(e.sid) ? session_gateway_[e.sid] : 0;
                           back.role = e.role;
                           back.reason = KSpawnReason::change_map;
                           source.post(std::move(back));
                           return;
                       }
                       KCmdSpawnPlayer cmd;
                       cmd.sid = e.sid;
                       cmd.conn_id = session_gateway_.count(e.sid) ? session_gateway_[e.sid] : 0;
                       cmd.role = e.role;
                       cmd.role.mutable_position()->set_map_id(e.map_id);
                       cmd.has_at = true;
                       cmd.at = target->world().to_local(e.pos);   // NewWorld passes absolute Mps coordinates
                       cmd.reason = KSpawnReason::change_map;
                       session_instance_[e.sid] = target;
                       target->post(std::move(cmd));
                   },
               },
               ev);
}

void KGameServer::send_stats()
{
    const auto tick_snapshot = profile_.total().snapshot();
    pb::ZoneStats st;
    st.set_zone_id(cfg_.world.zone_id);
    st.set_tick(clock_.tick());
    st.set_players(static_cast<std::uint32_t>(session_instance_.size()));
    st.set_entities(static_cast<std::uint32_t>(total_entities()));
    st.set_tick_ms_avg(static_cast<std::uint32_t>(tick_snapshot.avg_ms));
    st.set_tick_ms_max(static_cast<std::uint32_t>(tick_snapshot.max_ms));
    for (auto& [id, gw] : gateways_) {
        if (gw.ready) net::send(*gw.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_STATS), st);
    }
    // per map phase costs of the busiest map: where the time goes (SPEC 51, 53, 96)
    const KMapInstance* busiest = nullptr;
    for (const auto& inst : instances_) {
        if (busiest == nullptr || inst->avg_tick_ms() > busiest->avg_tick_ms()) busiest = inst.get();
    }
    std::string phases;
    if (busiest != nullptr) {
        const auto& m = core::metrics();
        for (const core::TickPhase ph : {core::TickPhase::drain_network, core::TickPhase::ai, core::TickPhase::interest,
                                         core::TickPhase::snapshot, core::TickPhase::spatial_update}) {
            const auto snap = core::metrics().timing(fmt::format("map.{}.tick.{}", busiest->map_id(), core::phase_name(ph))).snapshot();
            if (snap.count == 0) continue;
            if (!phases.empty()) phases += " ";
            phases += fmt::format("{}:{:.2f}ms", core::phase_name(ph), snap.avg_ms);
        }
        (void)m;
    }
    // per worker and per map numbers: which worker is hot, which map costs what (SPEC 52, 53)
    std::string per_worker;
    for (const KWorkerLoad& load : worker_loads()) {
        if (!per_worker.empty()) per_worker += " ";
        per_worker += fmt::format("w{}:{:.2f}ms/{}maps/{}p", load.worker, load.cost_ms, load.instances, load.players);
    }
    std::size_t awake = 0;
    std::uint64_t viewers_capped = 0;
    jx::zone::KSubWorld::LookStats looks;   // what the interest pass did since the last stats line
    for (const auto& inst : instances_) {
        awake += inst->world().awake_entities();
        viewers_capped += inst->world().viewers_capped();
        const auto& ls = inst->world().look_stats();
        looks.looks += ls.looks;
        looks.looks_idle += ls.looks_idle;
        looks.known_checked += ls.known_checked;
        looks.candidates += ls.candidates;
        looks.learned += ls.learned;
        looks.forgotten += ls.forgotten;
        inst->world().reset_look_stats();
    }
    const std::string look_line = fmt::format("{} looks, {} idle, {} known/look, {} candidates/look, {} learned, {} forgotten", looks.looks,
                                              looks.looks_idle, looks.known_checked / std::max<std::uint64_t>(1, looks.looks),
                                              looks.candidates / std::max<std::uint64_t>(1, looks.looks), looks.learned, looks.forgotten);
    const ProcessUsage usage = process_usage();   // what this many players actually cost (SPEC 55)
    // How much the gateway links are behind: an ack queued behind a megabyte of world traffic is
    // an ack the player waits for, so this is the number that explains a slow "enter world".
    std::size_t link_queued = 0;
    for (const auto& [conn_id, gw] : gateways_) {
        if (gw.conn) link_queued = std::max(link_queued, gw.conn->queued_bytes());
    }
    log::info("zone.tick", "stats",
              {log::kv("tick", clock_.tick()), log::kv("players", session_instance_.size()),
               log::kv("entities", total_entities()), log::kv("awake", awake), log::kv("maps", instances_.size()), log::kv("gateways", gateways_.size()),
               log::kv("tick_ms_avg", fmt::format("{:.2f}", tick_snapshot.avg_ms)),
               log::kv("tick_ms_p95", fmt::format("{:.2f}", tick_snapshot.p95_ms)),
               log::kv("tick_ms_p99", fmt::format("{:.2f}", tick_snapshot.p99_ms)),
               log::kv("tick_ms_max", fmt::format("{:.2f}", tick_snapshot.max_ms)),
               log::kv("sim_workers", workers_), log::kv("workers", per_worker),
               log::kv("busiest_map", busiest != nullptr ? busiest->map_id() : 0u), log::kv("phases", phases),
               log::kv("dropped", fixed_.dropped()),
               log::kv("link_kb", link_queued / 1024), log::kv("link_dropped", link_dropped_),
               log::kv("viewers_capped", viewers_capped), log::kv("looks", look_line),
               log::kv("rss_mb", usage.rss_bytes / (1024 * 1024)),
               log::kv("peak_rss_mb", usage.peak_rss_bytes / (1024 * 1024)),
               log::kv("cpu_s", fmt::format("{:.1f}", static_cast<double>(usage.cpu_ms) / 1000.0))});
    profile_.total().reset();
}

// ---- tick ----------------------------------------------------------------------------------

void KGameServer::schedule_tick()
{
    timer_.expires_after(std::chrono::duration_cast<std::chrono::steady_clock::duration>(clock_.rate().step()));
    timer_.async_wait([this](const std::error_code& ec) {
        if (ec || !running_) return;
        run_ticks();
        schedule_tick();
    });
}

void KGameServer::run_ticks()
{
    const Nanos now = steady_now();
    const Nanos elapsed = now - last_update_;
    last_update_ = now;
    const std::uint32_t due = fixed_.pending(elapsed);
    for (std::uint32_t i = 0; i < due; ++i) {
        clock_.advance();
        tick_once();
    }
}

void KGameServer::tick_once()
{
    {
        auto whole = profile_.whole();
        // 1. the simulation: every worker ticks the instances it owns, in parallel.  Two workers
        //    never touch the same instance, so no world state is shared (SPEC 6, 7, 73).
        {
            auto phase = profile_.phase(core::TickPhase::movement);
            if (workers_ <= 1 || buckets_.size() <= 1) {
                for (KMapInstance* m : instance_ptrs_) m->tick();
            } else {
                jobs_->parallel_for(buckets_.size(),
                                    [this](std::size_t w) {
                                        for (KMapInstance* m : buckets_[w]) m->tick();
                                    },
                                    1);
            }
        }
        // 2. what the worlds want from the outside: acks, saves, map transfers (server thread only)
        {
            auto phase = profile_.phase(core::TickPhase::entity_transfer);
            process_events();
        }
        // 3. the packets the worlds produced go to the gateways
        {
            auto phase = profile_.phase(core::TickPhase::network_send);
            flush_outbox();
        }
    }

    const std::uint64_t tick = clock_.tick();
    if (cfg_.stats_interval_s > 0 && tick - last_stats_tick_ >= static_cast<std::uint64_t>(cfg_.stats_interval_s) * cfg_.world.tick_hz) {
        last_stats_tick_ = tick;
        send_stats();
    }
    if (cfg_.save_interval_s > 0) {
        // Every player is saved once per interval, each in its own tick - (tick + sid) % period
        // picks the tick - instead of everybody together every save_interval_s.  Saved together,
        // 20 000 players are 20 000 database writes queued in the same 55 ms and a gateway link
        // full of PlayerSave for seconds; spread, they are ~19 writes a tick, all day long.  (The
        // owner's rule for a 20 000 player server: save per character, never in a burst.)
        const std::uint64_t period = static_cast<std::uint64_t>(cfg_.save_interval_s) * cfg_.world.tick_hz;
        for (const auto& [sid, inst] : session_instance_) {
            if ((tick + sid) % period == 0) inst->post(KCmdSaveRequest{sid, false});
        }
    }
    // the scheduler may move a map to a colder worker between two ticks (SPEC 8, 9)
    if (workers_ > 1 && cfg_.rebalance_interval_s > 0 &&
        tick - last_rebalance_tick_ >= static_cast<std::uint64_t>(cfg_.rebalance_interval_s) * cfg_.world.tick_hz) {
        last_rebalance_tick_ = tick;
        auto phase = profile_.phase(core::TickPhase::metrics);
        if (scheduler_.rebalance(instance_ptrs_) > 0) rebuild_buckets();
    }
}

} // namespace jx::zone
