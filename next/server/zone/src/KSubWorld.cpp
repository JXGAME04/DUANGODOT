#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cmath>

#include "jx/log.hpp"
#include "jx/msg.pb.h"

namespace jx::zone {
namespace {

std::int64_t isqrt(std::int64_t v) noexcept
{
    if (v <= 0) return 0;
    auto r = static_cast<std::int64_t>(std::sqrt(static_cast<double>(v)));
    while (r > 0 && r * r > v) --r;
    while ((r + 1) * (r + 1) <= v) ++r;
    return r;
}

pb::EntityType to_pb(KNpcKind k) noexcept
{
    switch (k) {
    case KNpcKind::player: return pb::ENTITY_PLAYER;
    case KNpcKind::npc: return pb::ENTITY_NPC;
    case KNpcKind::monster: return pb::ENTITY_MONSTER;
    case KNpcKind::drop: return pb::ENTITY_DROP;
    }
    return pb::ENTITY_UNKNOWN;
}

void set_vec(pb::Vec2* v, Pos p)
{
    v->set_x(p.x);
    v->set_y(p.y);
}

} // namespace

KSubWorld::KSubWorld(KSubWorldConfig cfg)
    : cfg_(std::move(cfg)), grid_(cfg_.cell_size, cfg_.view_cells), ids_(1), rng_(cfg_.seed)
{
    if (cfg_.tick_hz == 0) cfg_.tick_hz = 20;
    if (cfg_.map) {
        cfg_.width = cfg_.map->scene_w;
        cfg_.height = cfg_.map->scene_h;
        if (!cfg_.spawn_from_config) cfg_.spawn_point = cfg_.map->spawn;
        cfg_.spawn_point = cfg_.map->nearest_walkable(clamp(cfg_.spawn_point));
        grid_ = KRegionGrid(cfg_.cell_size, cfg_.view_cells);
        if (cfg_.map_npcs) {
            for (const KNpcPlacement& n : cfg_.map->npcs) {
                spawn_npc(n.name, n.pos, n.template_id, 0, KNpcKind::npc);
            }
            take_outbox();   // nobody is listening yet
            log::info("zone", "map npcs placed", {log::kv("count", cfg_.map->npcs.size())});
        }
    }
}

Pos KSubWorld::clamp(Pos p) const noexcept
{
    return Pos{std::clamp(p.x, 0, cfg_.width - 1), std::clamp(p.y, 0, cfg_.height - 1)};
}

const KNpc* KSubWorld::find_entity(EntityId id) const
{
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : &it->second;
}

const KNpc* KSubWorld::find_player(std::uint64_t sid) const
{
    const auto it = players_.find(sid);
    return it == players_.end() ? nullptr : find_entity(it->second);
}

std::vector<std::uint64_t> KSubWorld::session_ids() const
{
    std::vector<std::uint64_t> out;
    out.reserve(players_.size());
    for (const auto& [sid, id] : players_) out.push_back(sid);
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<Packet> KSubWorld::take_outbox()
{
    std::vector<Packet> out;
    out.swap(outbox_);
    return out;
}

void KSubWorld::emit(std::vector<std::uint64_t> sids, std::uint16_t msg_id, const google::protobuf::MessageLite& msg)
{
    if (sids.empty()) return;
    Packet p;
    p.sids = std::move(sids);
    p.msg_id = msg_id;
    msg.SerializeToString(&p.payload);
    outbox_.push_back(std::move(p));
}

void KSubWorld::viewers_of(Cell c, std::vector<std::uint64_t>& sids, EntityId exclude) const
{
    grid_.for_each_in_view(c, [&](EntityId id) {
        if (id == exclude) return;
        const auto it = entities_.find(id);
        if (it != entities_.end() && it->second.kind == KNpcKind::player && it->second.sid != 0) {
            sids.push_back(it->second.sid);
        }
    });
}

void KSubWorld::fill_info(const KNpc& e, pb::EntityInfo& out) const
{
    out.set_entity_id(e.id.value);
    out.set_entity_type(to_pb(e.kind));
    out.set_name(e.name);
    set_vec(out.mutable_pos(), e.pos());
    set_vec(out.mutable_target(), e.moving ? e.destination() : e.pos());
    out.set_move_speed(e.speed);
    out.set_level(e.level);
    out.set_series(e.series);
    out.set_sex(e.sex);
    out.set_template_id(e.template_id);
    out.set_dir(e.dir);
    if (e.moving) {
        set_vec(out.add_path(), e.target());
        for (const Pos& p : e.path) set_vec(out.add_path(), p);
    }
}

pb::Result KSubWorld::spawn_player(std::uint64_t sid, const pb::RoleData& role, EntityId& entity_out, Pos& pos_out)
{
    if (sid == 0) return pb::RESULT_BAD_REQUEST;
    if (players_.contains(sid)) return pb::RESULT_WRONG_STATE;
    if (players_.size() >= cfg_.max_players) return pb::RESULT_FULL;

    KNpc e;
    e.id = ids_.next<EntityId>();
    e.kind = KNpcKind::player;
    e.name = role.name();
    e.level = role.level() == 0 ? 1u : role.level();
    e.series = role.series();
    e.sex = role.sex();
    e.sid = sid;
    e.player_id = role.player_id();
    e.speed = role.stats().move_speed() > 0 ? static_cast<std::uint32_t>(role.stats().move_speed()) : cfg_.default_speed;

    Pos start = cfg_.spawn_point;
    if (role.has_position() && role.position().zone_id() == cfg_.zone_id && role.position().has_pos()) {
        start = clamp(Pos{role.position().pos().x(), role.position().pos().y()});
    }
    if (cfg_.map) start = cfg_.map->nearest_walkable(start);
    e.set_pos(start);
    const EntityId id = e.id;
    grid_.insert(id, start);
    entities_.emplace(id, std::move(e));
    players_[sid] = id;
    roles_[sid] = role;

    const KNpc& self = entities_.at(id);
    const Cell cell = grid_.cell_of(start);

    // everything the newcomer can see (including itself)
    pb::EntitySpawn visible;
    grid_.for_each_in_view(cell, [&](EntityId other) { fill_info(entities_.at(other), *visible.add_entities()); });
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), visible);

    // the newcomer for everyone already there
    scratch_sids_.clear();
    viewers_of(cell, scratch_sids_, id);
    if (!scratch_sids_.empty()) {
        pb::EntitySpawn me;
        fill_info(self, *me.add_entities());
        emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), me);
    }

    entity_out = id;
    pos_out = start;
    log::ScopedContext ctx(log::Context{sid, role.player_id(), cfg_.zone_id, tick_});
    log::info("zone", "player spawned", {log::kv("entity", id), log::kv("name", role.name()), log::kv("x", start.x), log::kv("y", start.y),
                                         log::kv("visible", visible.entities_size())});
    return pb::RESULT_OK;
}

bool KSubWorld::remove_player(std::uint64_t sid)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    const EntityId id = pit->second;
    const auto eit = entities_.find(id);
    if (eit != entities_.end()) {
        const Cell cell = grid_.cell_of(eit->second.pos());
        scratch_sids_.clear();
        viewers_of(cell, scratch_sids_, id);
        if (!scratch_sids_.empty()) {
            pb::EntityDespawn gone;
            gone.add_entity_ids(id.value);
            emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_DESPAWN), gone);
        }
        log::ScopedContext ctx(log::Context{sid, eit->second.player_id, cfg_.zone_id, tick_});
        log::info("zone", "player removed", {log::kv("entity", id), log::kv("x", eit->second.pos().x), log::kv("y", eit->second.pos().y)});
        grid_.remove(id);
        entities_.erase(eit);
    }
    players_.erase(pit);
    roles_.erase(sid);
    return true;
}

void KSubWorld::emit_move(const KNpc& e)
{
    pb::EntityMove mv;
    mv.set_entity_id(e.id.value);
    set_vec(mv.mutable_pos(), e.pos());
    set_vec(mv.mutable_target(), e.moving ? e.destination() : e.pos());
    mv.set_move_speed(e.speed);
    mv.set_tick(tick_);
    if (e.moving) {
        set_vec(mv.add_path(), e.target());
        for (const Pos& p : e.path) set_vec(mv.add_path(), p);
    }

    const Cell cell = grid_.cell_of(e.pos());
    scratch_sids_.clear();
    viewers_of(cell, scratch_sids_, e.id);
    if (!scratch_sids_.empty()) {
        mv.set_seq(0);
        emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_MOVE), mv);
    }
    if (e.kind == KNpcKind::player && e.sid != 0) {
        mv.set_seq(e.move_seq);
        emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_ENTITY_MOVE), mv);
    }
}

bool KSubWorld::move_request(std::uint64_t sid, Pos target, std::uint32_t seq)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    KNpc& e = entities_.at(pit->second);
    e.move_seq = seq;
    Pos dest = clamp(target);
    std::size_t waypoints = 1;
    if (cfg_.map) {
        dest = cfg_.map->nearest_walkable(dest);
        std::vector<Pos> path = cfg_.map->find_path(e.pos(), dest);
        waypoints = path.size();
        e.set_path(std::move(path));   // empty = unreachable: stop where we are
    } else {
        e.set_target(dest);
    }
    log::ScopedContext ctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    log::trace("zone.move", "move request", {log::kv("entity", e.id), log::kv("tx", dest.x), log::kv("ty", dest.y), log::kv("seq", seq),
                                              log::kv("waypoints", waypoints)});
    emit_move(e);
    return true;
}

bool KSubWorld::chat(std::uint64_t sid, std::string_view text)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    const KNpc& e = entities_.at(pit->second);
    pb::ChatMsg msg;
    msg.set_entity_id(e.id.value);
    msg.set_name(e.name);
    msg.set_text(std::string(text));
    scratch_sids_.clear();
    viewers_of(grid_.cell_of(e.pos()), scratch_sids_, EntityId{});   // includes the sender
    emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_CHAT_MSG), msg);
    log::ScopedContext ctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    log::debug("zone.chat", "chat", {log::kv("entity", e.id), log::kv("len", text.size()), log::kv("receivers", scratch_sids_.size())});
    return true;
}

EntityId KSubWorld::spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius, KNpcKind kind)
{
    KNpc e;
    e.id = ids_.next<EntityId>();
    e.kind = kind;
    e.name = std::move(name);
    e.template_id = template_id;
    e.speed = cfg_.default_speed / 2;
    e.wander_radius = wander_radius;
    e.home = clamp(pos);
    if (cfg_.map) e.home = cfg_.map->nearest_walkable(e.home);
    e.set_pos(e.home);
    e.next_wander_tick = tick_ + static_cast<std::uint64_t>(rng_() % (cfg_.tick_hz * 5 + 1));
    const EntityId id = e.id;
    grid_.insert(id, e.home);
    entities_.emplace(id, std::move(e));

    const KNpc& self = entities_.at(id);
    scratch_sids_.clear();
    viewers_of(grid_.cell_of(self.pos()), scratch_sids_, id);
    if (!scratch_sids_.empty()) {
        pb::EntitySpawn me;
        fill_info(self, *me.add_entities());
        emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), me);
    }
    return id;
}

void KSubWorld::wander(KNpc& e)
{
    const std::int32_t r = e.wander_radius;
    const auto dx = static_cast<std::int32_t>(rng_() % static_cast<std::uint32_t>(2 * r + 1)) - r;
    const auto dy = static_cast<std::int32_t>(rng_() % static_cast<std::uint32_t>(2 * r + 1)) - r;
    Pos dest = clamp(Pos{e.home.x + dx, e.home.y + dy});
    if (cfg_.map) {
        dest = cfg_.map->nearest_walkable(dest);
        e.set_path(cfg_.map->find_path(e.pos(), dest, 2000));
    } else {
        e.set_target(dest);
    }
    e.next_wander_tick = tick_ + cfg_.tick_hz * 2 + static_cast<std::uint64_t>(rng_() % (cfg_.tick_hz * 6));
    if (e.moving) emit_move(e);
}

void KSubWorld::on_cell_change(KNpc& e, Cell from, Cell to)
{
    grid_.view_diff(from, to, scratch_entered_, scratch_left_);

    pb::EntitySpawn appear;      // what e starts to see
    pb::EntityDespawn vanish;    // what e stops seeing
    pb::EntitySpawn me_spawn;    // e for the players that start to see it
    pb::EntityDespawn me_gone;
    fill_info(e, *me_spawn.add_entities());
    me_gone.add_entity_ids(e.id.value);

    std::vector<std::uint64_t> new_viewers, old_viewers;
    for (const EntityId other : scratch_entered_) {
        if (other == e.id) continue;
        const auto it = entities_.find(other);
        if (it == entities_.end()) continue;
        fill_info(it->second, *appear.add_entities());
        if (it->second.kind == KNpcKind::player && it->second.sid != 0) new_viewers.push_back(it->second.sid);
    }
    for (const EntityId other : scratch_left_) {
        if (other == e.id) continue;
        const auto it = entities_.find(other);
        if (it == entities_.end()) continue;
        vanish.add_entity_ids(other.value);
        if (it->second.kind == KNpcKind::player && it->second.sid != 0) old_viewers.push_back(it->second.sid);
    }
    if (e.kind == KNpcKind::player && e.sid != 0) {
        if (appear.entities_size() > 0) emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), appear);
        if (vanish.entity_ids_size() > 0) emit({e.sid}, static_cast<std::uint16_t>(pb::G2C_ENTITY_DESPAWN), vanish);
    }
    if (!new_viewers.empty()) emit(std::move(new_viewers), static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), me_spawn);
    if (!old_viewers.empty()) emit(std::move(old_viewers), static_cast<std::uint16_t>(pb::G2C_ENTITY_DESPAWN), me_gone);
}

void KSubWorld::tick()
{
    ++tick_;
    // deterministic iteration order regardless of hash layout
    scratch_ids_.clear();
    for (const auto& [id, e] : entities_) scratch_ids_.push_back(id);
    std::sort(scratch_ids_.begin(), scratch_ids_.end());

    std::vector<EntityId> moved, arrived;
    for (const EntityId id : scratch_ids_) {
        KNpc& e = entities_.at(id);
        if (e.kind != KNpcKind::player && e.wander_radius > 0 && !e.moving && tick_ >= e.next_wander_tick) wander(e);
        if (!e.moving) continue;
        std::int64_t budget = static_cast<std::int64_t>(e.speed) * kSub / cfg_.tick_hz;   // sub-units this tick
        while (budget > 0 && e.moving) {
            const std::int64_t dx = e.tx - e.fx;
            const std::int64_t dy = e.ty - e.fy;
            const std::int64_t dist2 = dx * dx + dy * dy;
            if (dist2 <= budget * budget) {
                const std::int64_t d = isqrt(dist2);
                e.fx = e.tx;
                e.fy = e.ty;
                budget -= d;
                if (!e.next_waypoint()) {
                    arrived.push_back(id);
                    break;
                }
            } else {
                const std::int64_t d = isqrt(dist2);
                e.fx += dx * budget / d;
                e.fy += dy * budget / d;
                budget = 0;
            }
        }
        moved.push_back(id);
    }
    for (const EntityId id : moved) {
        KNpc& e = entities_.at(id);
        Cell from, to;
        if (grid_.move(id, e.pos(), from, to)) on_cell_change(e, from, to);
    }
    for (const EntityId id : arrived) emit_move(entities_.at(id));
}

bool KSubWorld::role_snapshot(std::uint64_t sid, pb::RoleData& out) const
{
    const auto rit = roles_.find(sid);
    if (rit == roles_.end()) return false;
    out = rit->second;
    if (const KNpc* e = find_player(sid)) {
        out.mutable_position()->set_zone_id(cfg_.zone_id);
        set_vec(out.mutable_position()->mutable_pos(), e->pos());
        out.set_level(e->level);
    }
    return true;
}

} // namespace jx::zone
