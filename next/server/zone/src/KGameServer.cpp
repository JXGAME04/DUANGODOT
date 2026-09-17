#include "jx/zone/KGameServer.h"

#include <chrono>

#include "jx/internal.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/net/proto.hpp"

namespace jx::zone {

using namespace std::chrono_literals;

KGameServer::KGameServer(asio::io_context& io, KGameServerConfig cfg)
    : io_(io), cfg_(std::move(cfg)), listener_(io, frame::kMaxInternalPayload), timer_(io),
      step_(cfg_.world.tick_hz, cfg_.max_ticks_per_update)
{
    // KSubWorldSet: one KSubWorld per hosted map; the first is the default
    if (cfg_.worlds.empty()) cfg_.worlds.push_back(cfg_.world);
    for (const KSubWorldConfig& wc : cfg_.worlds) {
        auto world = std::make_unique<KSubWorld>(wc);
        if (world_by_map_.contains(world->map_id())) {
            log::warn("boot", "map hosted twice, second copy ignored", {log::kv("map", world->map_id())});
            continue;
        }
        world_by_map_[world->map_id()] = world.get();
        worlds_.push_back(std::move(world));
    }
}

KSubWorld* KGameServer::world_of_map(std::uint32_t map_id) noexcept
{
    const auto it = world_by_map_.find(map_id);
    return it == world_by_map_.end() ? nullptr : it->second;
}

KSubWorld* KGameServer::world_of_session(std::uint64_t sid) noexcept
{
    const auto it = session_world_.find(sid);
    return it == session_world_.end() ? nullptr : it->second;
}

std::size_t KGameServer::total_entities() const noexcept
{
    std::size_t n = 0;
    for (const auto& w : worlds_) n += w->entity_count();
    return n;
}

std::error_code KGameServer::start()
{
    if (const auto ec = listener_.open(cfg_.listen_address, cfg_.port)) {
        log::error("boot", "cannot listen", {log::kv("addr", cfg_.listen_address), log::kv("port", cfg_.port), log::kv("error", ec.message())});
        return ec;
    }
    listener_.start([this](net::Connection::Ptr c) { on_accept(std::move(c)); });
    running_ = true;
    last_update_ = steady_now();
    schedule_tick();
    log::info("boot", "zone listening", {log::kv("addr", cfg_.listen_address), log::kv("port", listener_.port()),
                                         log::kv("zone", cfg_.world.zone_id), log::kv("name", cfg_.world.name),
                                         log::kv("tick_hz", cfg_.world.tick_hz)});
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
    log::info("boot", "zone stopped", {log::kv("tick", world().tick_count()), log::kv("players", session_world_.size())});
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
            if (KSubWorld* w = world_of_session(sit->first)) w->remove_player(sit->first);
            session_world_.erase(sit->first);
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
    flush_outbox();
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
    flush_outbox();
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
    net::send(*gw.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_HELLO_ACK), ack);
    log::info("net", "gateway ready", {log::kv("conn", gw.conn->id()), log::kv("gateway", gw.id)});
}

void KGameServer::handle_session_open(Gateway& gw, const frame::View& view)
{
    pb::SessionOpen open;
    if (!net::parse(view, open)) {
        log::warn("net", "bad SessionOpen", {log::kv("conn", gw.conn->id())});
        return;
    }
    pb::SessionOpenAck ack;
    ack.set_sid(open.sid());
    EntityId entity;
    Pos pos;
    // the saved map when this zone hosts it (g_SubWorldSet.SearchWorld), else the default map
    KSubWorld* target = open.role().has_position() ? world_of_map(open.role().position().map_id()) : nullptr;
    if (target == nullptr) target = &world();
    const pb::Result result = target->spawn_player(open.sid(), open.role(), entity, pos);
    ack.set_result(result);
    if (result == pb::RESULT_OK) {
        session_gateway_[open.sid()] = gw.conn->id();
        session_world_[open.sid()] = target;
        ack.set_map_id(target->map_id());
        ack.set_scene_w(static_cast<std::uint32_t>(target->config().width));
        ack.set_scene_h(static_cast<std::uint32_t>(target->config().height));
        ack.set_entity_id(entity.value);
        ack.mutable_pos()->set_x(pos.x);
        ack.mutable_pos()->set_y(pos.y);
    } else {
        log::warn("zone", "session open rejected", {log::kv("sid", open.sid()), log::kv("result", static_cast<int>(result))});
    }
    net::send(*gw.conn, static_cast<std::uint16_t>(pb::ZG_SESSION_OPEN_ACK), ack);
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
    send_save(close.sid(), true);
    if (KSubWorld* w = world_of_session(close.sid())) w->remove_player(close.sid());
    session_world_.erase(close.sid());
    session_gateway_.erase(it);
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
    KSubWorld* w = world_of_session(cp.sid());
    if (w == nullptr) return;
    switch (cp.msg_id()) {
    case pb::C2G_MOVE: {
        pb::MoveReq req;
        if (!net::parse(cp.payload(), req)) break;
        w->move_request(cp.sid(), Pos{req.target().x(), req.target().y()}, req.seq());
        break;
    }
    case pb::C2G_CHAT: {
        pb::ChatReq req;
        if (!net::parse(cp.payload(), req)) break;
        w->chat(cp.sid(), req.text());
        break;
    }
    case pb::C2G_ATTACK: {
        pb::AttackReq req;
        if (!net::parse(cp.payload(), req)) break;
        w->attack_request(cp.sid(), jx::EntityId{req.target()}, req.seq());
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
    for (auto& world : worlds_) {
        if (world->outbox().empty()) continue;
        for (Packet& p : world->take_outbox()) {
            // group the sessions of one packet by the gateway that owns them
            std::unordered_map<std::uint64_t, pb::ZonePacket> per_gateway;
            for (const std::uint64_t sid : p.sids) {
                const auto it = session_gateway_.find(sid);
                if (it == session_gateway_.end()) continue;
                per_gateway[it->second].add_sids(sid);
            }
            for (auto& [conn_id, zp] : per_gateway) {
                const auto git = gateways_.find(conn_id);
                if (git == gateways_.end() || !git->second.conn) continue;
                zp.set_msg_id(p.msg_id);
                zp.set_payload(p.payload);
                net::send(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_PACKET), zp);
            }
        }
    }
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

// KNpc::ChangeWorld between two worlds of this zone: leave the old map (despawn for its viewers),
// tell the client which bundle to load, then spawn in the new map (its EntitySpawn follows).
void KGameServer::process_world_changes()
{
    for (auto& source : worlds_) {
        for (const KWorldChange& ch : source->take_world_changes()) {
            KSubWorld* target = world_of_map(ch.map_id);
            if (target == nullptr) {   // 不在这台服务器上: TobeExchangeServer is not there yet
                log::warn("zone.trap", "map not hosted here, player stays", {log::kv("sid", ch.sid), log::kv("map", ch.map_id)});
                continue;
            }
            if (world_of_session(ch.sid) != source.get()) continue;
            pb::RoleData role;
            if (!source->role_snapshot(ch.sid, role)) continue;
            source->remove_player(ch.sid);
            flush_outbox();   // the despawn for the old neighbours goes out before anything new
            EntityId entity;
            Pos pos;
            role.mutable_position()->set_map_id(ch.map_id);
            const Pos at = target->to_local(ch.pos);   // NewWorld passes absolute Mps coordinates
            const pb::Result result = target->spawn_player(ch.sid, role, entity, pos, &at);
            if (result != pb::RESULT_OK) {
                log::error("zone.trap", "spawn in the new map failed, back to the old one", {log::kv("sid", ch.sid), log::kv("map", ch.map_id), log::kv("result", static_cast<int>(result))});
                source->spawn_player(ch.sid, role, entity, pos);
                continue;
            }
            session_world_[ch.sid] = target;
            pb::ChangeMap change;
            change.set_map_id(target->map_id());
            change.mutable_pos()->set_x(pos.x);
            change.mutable_pos()->set_y(pos.y);
            change.set_scene_w(static_cast<std::uint32_t>(target->config().width));
            change.set_scene_h(static_cast<std::uint32_t>(target->config().height));
            change.set_entity_id(entity.value);
            send_to_session(ch.sid, static_cast<std::uint16_t>(pb::G2C_CHANGE_MAP), change);   // before the spawn packets
            log::info("zone.trap", "player changed map", {log::kv("sid", ch.sid), log::kv("from", source->map_id()), log::kv("to", target->map_id()),
                                                         log::kv("x", pos.x), log::kv("y", pos.y), log::kv("entity", entity)});
        }
    }
}

void KGameServer::send_save(std::uint64_t sid, bool final)
{
    const auto it = session_gateway_.find(sid);
    if (it == session_gateway_.end()) return;
    const auto git = gateways_.find(it->second);
    if (git == gateways_.end()) return;
    pb::PlayerSave save;
    save.set_sid(sid);
    save.set_final(final);
    KSubWorld* w = world_of_session(sid);
    if (w == nullptr || !w->role_snapshot(sid, *save.mutable_role())) return;
    net::send(*git->second.conn, static_cast<std::uint16_t>(pb::ZG_PLAYER_SAVE), save);
}

void KGameServer::send_stats()
{
    pb::ZoneStats st;
    st.set_zone_id(cfg_.world.zone_id);
    st.set_tick(world().tick_count());
    st.set_players(static_cast<std::uint32_t>(session_world_.size()));
    st.set_entities(static_cast<std::uint32_t>(total_entities()));
    const double avg = tick_samples_ ? tick_ms_sum_ / static_cast<double>(tick_samples_) : 0.0;
    st.set_tick_ms_avg(static_cast<std::uint32_t>(avg));
    st.set_tick_ms_max(static_cast<std::uint32_t>(tick_ms_max_));
    for (auto& [id, gw] : gateways_) {
        if (gw.ready) net::send(*gw.conn, static_cast<std::uint16_t>(pb::ZG_ZONE_STATS), st);
    }
    log::info("zone.tick", "stats", {log::kv("tick", world().tick_count()), log::kv("players", session_world_.size()),
                                     log::kv("entities", total_entities()), log::kv("maps", worlds_.size()), log::kv("gateways", gateways_.size()),
                                     log::kv("tick_ms_avg", fmt::format("{:.2f}", avg)), log::kv("tick_ms_max", fmt::format("{:.2f}", tick_ms_max_)),
                                     log::kv("dropped", step_.dropped())});
    tick_ms_sum_ = tick_ms_max_ = 0;
    tick_samples_ = 0;
}

// ---- tick ----------------------------------------------------------------------------------

void KGameServer::schedule_tick()
{
    timer_.expires_after(std::chrono::duration_cast<std::chrono::steady_clock::duration>(step_.step()));
    timer_.async_wait([this](const std::error_code& ec) {
        if (ec || !running_) return;
        run_ticks();
        schedule_tick();
    });
}

void KGameServer::run_ticks()
{
    const Nanos now = steady_now();
    const std::uint32_t n = step_.update(now - last_update_);
    last_update_ = now;
    for (std::uint32_t i = 0; i < n; ++i) {
        const Nanos t0 = steady_now();
        for (auto& w : worlds_) w->tick();
        process_world_changes();
        flush_outbox();
        const double ms = static_cast<double>((steady_now() - t0).count()) / 1e6;
        tick_ms_sum_ += ms;
        if (ms > tick_ms_max_) tick_ms_max_ = ms;
        ++tick_samples_;

        const std::uint64_t tick = world().tick_count();
        if (cfg_.stats_interval_s > 0 && tick - last_stats_tick_ >= static_cast<std::uint64_t>(cfg_.stats_interval_s) * cfg_.world.tick_hz) {
            last_stats_tick_ = tick;
            send_stats();
        }
        if (cfg_.save_interval_s > 0 && tick - last_save_tick_ >= static_cast<std::uint64_t>(cfg_.save_interval_s) * cfg_.world.tick_hz) {
            last_save_tick_ = tick;
            for (const auto& [sid, w] : session_world_) send_save(sid, false);
        }
    }
}

} // namespace jx::zone
