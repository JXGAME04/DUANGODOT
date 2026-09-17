#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cmath>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KNpcAI.h"

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
                // NPCKIND of the old GameDataDef.h: 0 = kind_normal (a monster); everything else
                // (partner, dialoger, bird, mouse) is a friendly npc
                const KNpcKind kind = n.kind == 0 ? KNpcKind::monster : KNpcKind::npc;
                const EntityId id = spawn_npc(n.name, n.pos, n.template_id, 0, kind);
                if (auto it = entities_.find(id); it != entities_.end()) {
                    it->second.dir = static_cast<std::uint32_t>(n.dir & 63);
                    it->second.level = static_cast<std::uint32_t>(std::max(1, n.level));
                    it->second.series = static_cast<std::uint32_t>(n.series);
                    apply_template(it->second);   // skill levels depend on the level (KNpcTemplate::InitNpcLevelData)
                    it->second.npc_kind = n.kind;   // KNpcSet::Add: m_Kind / m_Camp come from the placement (KSNpcInfo)
                    it->second.camp = std::clamp(n.camp, 0, camp_num - 1);
                    it->second.current_camp = it->second.camp;
                }
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
    out.set_life(e.life);
    out.set_life_max(e.life_max);
    switch (e.doing) {
    case KDoing::attack: out.set_doing(pb::ACTION_ATTACK); break;
    case KDoing::hurt: out.set_doing(pb::ACTION_HURT); break;
    case KDoing::death: out.set_doing(pb::ACTION_DEATH); break;
    case KDoing::revive: out.set_doing(pb::ACTION_REVIVE); break;
    default: out.set_doing(pb::ACTION_STAND); break;
    }
    out.set_doing_frames(e.frame_total);
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
    e.npc_kind = kind_player;
    e.camp = camp_begin;   // BaseInfo.iteam of the old RoleData is not carried yet: every player is a beginner
    e.current_camp = camp_begin;
    // placeholders until KPlayer attributes and equipment set them (KPlayer::UpdateBaseData)
    e.min_damage = 10;
    e.max_damage = 20;
    e.attack_rating = 100;
    e.defend = 0;
    e.life_replenish = static_cast<int>((e.level + 5) / 6);   // KNpc.cpp:3971 for players
    e.name = role.name();
    e.level = role.level() == 0 ? 1u : role.level();
    e.series = role.series();
    e.sex = role.sex();
    e.sid = sid;
    e.player_id = role.player_id();
    e.speed = role.stats().move_speed() > 0 ? static_cast<std::uint32_t>(role.stats().move_speed()) : cfg_.default_speed;
    // BaseValue.ini of the old client ([Common] AttackFrame 18, HurtFrame 12); life and damage are
    // placeholders until the attribute system
    e.attack_frame = 18;
    e.hurt_frame = 12;
    e.death_frame = 15;
    e.hit_recover = 12;
    e.life_max = 100;
    e.life = e.life_max;
    e.min_damage = 5;
    e.max_damage = 10;

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
    // a corpse or a hit character cannot walk; walking interrupts an attack (KNpc::DoWalk)
    if (!e.alive() || e.doing == KDoing::hurt) return false;
    e.attack_target = EntityId{};
    if (e.doing == KDoing::attack) {
        e.doing = KDoing::stand;
        e.frame_cur = 0;
    }
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
    apply_template(e);
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
        // KNpc::Activate: m_LoopFrames++, ProcessState every GAME_UPDATE_TIME frames, then NpcAI.Activate
        // while m_ProcessAI, then the command / status of the frame
        ++e.loop_frames;
        if (e.loop_frames % kGameUpdateTime == 0) process_state(e);
        if (e.kind != KNpcKind::player && e.ai_mode != 0 && e.process_ai()) KNpcAI::activate(*this, e);
        update_action(e);
        if (e.kind != KNpcKind::player && e.ai_mode == 0 && e.wander_radius > 0 && !e.moving && e.doing == KDoing::stand && tick_ >= e.next_wander_tick) wander(e);
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

// ---- combat ---------------------------------------------------------------------------------

// KNpc::Load: the template's frame counts and (placeholder) life / damage.  Without a template
// table the old KNpc defaults apply, with a small life so test monsters can die.
void KSubWorld::apply_template(KNpc& e) const
{
    const KNpcTemplate* t = cfg_.templates ? cfg_.templates->find(e.template_id) : nullptr;
    if (t == nullptr) {
        e.life_max = e.kind == KNpcKind::monster ? 30u : 100u;
        e.life = e.life_max;
        return;
    }
    e.attack_frame = t->attack_frame;
    e.cast_frame = t->cast_frame;
    e.hurt_frame = t->hurt_frame;
    e.death_frame = t->death_frame;
    e.hit_recover = t->hit_recover;
    e.revive_frame = t->revive_frame;
    // KNpc::Init -> InitNpcLevelData -> LoadDataFromTemplate: the level data through the level script
    const KNpcLevelData& d = level_data_of(*t, static_cast<int>(std::max(1u, e.level)), static_cast<int>(e.series));
    e.level_data_from_script = d.from_script;
    e.min_damage = d.min_damage;
    e.max_damage = std::max(d.min_damage, d.max_damage);
    e.attack_rating = d.attack_rating;
    e.defend = d.defend;
    e.physics_resist = d.physics_resist;
    e.life_replenish = d.life_replenish;
    e.exp = d.exp;
    e.life_max = std::max(1u, d.life_max);
    e.life = e.life_max;
    if (e.kind != KNpcKind::player) {
        // the server side of KNpc::Init from the template: camp, ai and the skill list
        e.npc_kind = t->kind;
        e.camp = std::clamp(t->camp, 0, camp_num - 1);
        e.current_camp = e.camp;
        e.ai_mode = t->ai_mode;
        for (int i = 0; i < 10; ++i) e.ai_param[i] = t->ai_param[i];
        e.ai_max_time = std::max(1u, t->ai_max_time);
        e.vision_radius = t->vision_radius;
        e.active_radius = t->active_radius;
        e.current_active_radius = e.active_radius;
        e.walk_speed = t->walk_speed;
        e.run_speed = t->run_speed;
        e.speed = static_cast<std::uint32_t>(std::max(1, t->walk_speed)) * cfg_.tick_hz;   // ServeMove: WalkSpeed units per frame
        int max_radius = 0;
        for (int i = 1; i < 5; ++i) {
            const KNpcTemplateSkill& s = t->skills[i];
            KNpcSkillSlot& slot = e.skills[i];
            slot = KNpcSkillSlot{};
            if (s.id <= 0) continue;
            const int level = d.skill_level[i];   // Level1..4 through the level script
            if (level <= 0) continue;             // KSkillList::SetNpcSkill ignores a level of 0
            slot.id = s.id;
            slot.level = level;
            slot.known = s.known;
            slot.attack_radius = s.attack_radius;
            slot.melee = s.melee;
            slot.target_self = s.target_self;
            if (s.known && s.attack_radius > max_radius) max_radius = s.attack_radius;
        }
        e.ai_param[KNpcAI::kMaxAiParam - 1] = max_radius * max_radius;   // KNpc::Init: the reach of the farthest skill
    }
}

int KSubWorld::reach_of(const KNpc& e) const noexcept
{
    if (e.kind == KNpcKind::player) return kMeleeReach;
    return std::max(e.current_attack_radius, KNpcAI::kMiniAttackRange);
}

bool KSubWorld::in_reach(const KNpc& a, const KNpc& b) const noexcept
{
    const std::int64_t dx = a.pos().x - b.pos().x;
    const std::int64_t dy = a.pos().y - b.pos().y;
    const std::int64_t reach = reach_of(a);
    return dx * dx + dy * dy <= reach * reach;
}

bool KSubWorld::attack_request(std::uint64_t sid, EntityId target, std::uint32_t seq)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    KNpc& e = entities_.at(pit->second);
    if (!e.alive() || e.doing == KDoing::hurt) return false;
    const auto tit = entities_.find(target);
    if (tit == entities_.end() || tit->first == e.id || !tit->second.alive()) return false;
    if (tit->second.kind == KNpcKind::npc || tit->second.kind == KNpcKind::drop) return false;   // townsfolk cannot be attacked
    log::ScopedContext ctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    e.move_seq = seq;
    e.attack_target = target;
    e.approach_tries = 0;
    if (in_reach(e, tit->second)) {
        if (e.moving) {
            e.set_pos(e.pos());
            emit_move(e);
        }
        if (e.doing == KDoing::stand) start_attack(e, tit->second);
    } else {
        // like the old client: walk up to the target first, the swing starts on arrival
        approach(e, tit->second);
    }
    log::trace("zone.fight", "attack request", {log::kv("entity", e.id), log::kv("target", target), log::kv("seq", seq),
                                                 log::kv("in_reach", in_reach(e, tit->second))});
    return true;
}

// Walks toward a target that is out of reach (KNpc::Attack -> NewPath in the old core); a
// target that keeps running away is given up after a few tries.
void KSubWorld::approach(KNpc& e, const KNpc& target)
{
    if (e.approach_tries >= 5) {
        e.attack_target = EntityId{};
        return;
    }
    ++e.approach_tries;
    Pos dest = clamp(target.pos());
    if (cfg_.map) {
        dest = cfg_.map->nearest_walkable(dest);
        std::vector<Pos> path = cfg_.map->find_path(e.pos(), dest);
        if (path.empty()) {
            e.attack_target = EntityId{};
            return;
        }
        e.set_path(std::move(path));
    } else {
        e.set_target(dest);
    }
    emit_move(e);
}

// KNpc::DoAttack: the swing lasts AttackFrame * 100 / (100 + attack speed) frames.
void KSubWorld::start_attack(KNpc& e, KNpc& target)
{
    begin_action(e, target, std::max(1u, e.attack_frame * 100 / (100 + e.attack_speed)));
}

void KSubWorld::begin_action(KNpc& e, KNpc& target, std::uint32_t frames)
{
    e.doing = KDoing::attack;
    e.frame_total = frames;
    e.frame_cur = 0;
    e.attack_target = target.id;
    if (target.id != e.id) {
        const int d = g_GetDirIndex(e.pos().x, e.pos().y, target.pos().x, target.pos().y);
        if (d >= 0) e.dir = static_cast<std::uint32_t>(d);
    }
    emit_action(e, pb::ACTION_ATTACK, target.id);
}

// SendCommand(do_skill, skill, -1, target) -> KNpc::DoSkill on the old server: a melee skill swings
// (DoAttack, AttackFrame), any other one casts (do_magic, CastFrame); the effect lands at 60 % of
// the frames (OnSkill).  A cast on oneself is a heal.
void KSubWorld::cast_skill(KNpc& e, KNpc& target)
{
    if (e.doing == KDoing::attack) return;   // DoSkill: already casting
    if (e.active_skill_id == 0) return;      // nothing selected: ProcCommand finds no skill to cast
    if (e.moving) {
        e.set_pos(e.pos());
        emit_move(e);
    }
    const std::uint32_t base = e.active_skill_melee ? e.attack_frame : e.cast_frame;
    begin_action(e, target, std::max(1u, base * 100 / (100 + e.attack_speed)));
}

// SendCommand(do_walk) -> ProcCommand -> Goto -> NewPath + DoWalk.  The old npcs steer straight at
// the spot (KPathFinder::GetDir, stopping when blocked); here the map's path finder walks them
// around obstacles.  An unreachable or already reached spot ends in DoStand.
void KSubWorld::walk_to(KNpc& e, Pos dest)
{
    if (e.doing == KDoing::attack) {   // DoWalk overrides a swing
        e.doing = KDoing::stand;
        e.frame_cur = 0;
        e.attack_target = EntityId{};
    }
    dest = clamp(dest);
    if (cfg_.map) dest = cfg_.map->nearest_walkable(dest);
    if (dest == e.pos()) {
        do_stand(e);
        return;
    }
    if (cfg_.map) {
        std::vector<Pos> path = cfg_.map->find_path(e.pos(), dest, 4000);
        if (path.empty()) {
            do_stand(e);
            return;
        }
        e.set_path(std::move(path));
    } else {
        e.set_target(dest);
    }
    emit_move(e);
}

// SendCommand(do_stand) -> KNpc::DoStand
void KSubWorld::do_stand(KNpc& e)
{
    if (e.doing == KDoing::attack) {
        e.doing = KDoing::stand;
        e.frame_cur = 0;
        e.attack_target = EntityId{};
    }
    if (e.moving) {
        e.set_pos(e.pos());
        emit_move(e);
    }
}

KNpc* KSubWorld::find_mutable(EntityId id)
{
    if (!id) return nullptr;
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : &it->second;
}

bool KSubWorld::rand_percent(int percent) { return static_cast<int>(rng_() % 100) < percent; }

int KSubWorld::random(int n) { return n <= 0 ? 0 : static_cast<int>(rng_() % static_cast<std::uint32_t>(n)); }

// KNpcSet::GetRelation, server side: anything but two players goes through the camp table; the
// PK rules between players (exercise / enmity / normal PK state) are not simulated yet, so two
// players are never enemies.
int KSubWorld::relation(const KNpc& a, const KNpc& b) const noexcept
{
    if (a.id == b.id) return relation_self;
    const int k1 = a.kind == KNpcKind::player ? kind_player : a.npc_kind;
    const int k2 = b.kind == KNpcKind::player ? kind_player : b.npc_kind;
    const int r = g_GenOneRelation(k1, k2, a.current_camp, b.current_camp);
    if (k1 == kind_player && k2 == kind_player && r == relation_enemy) return relation_none;
    return r;
}

bool KSubWorld::teleport(EntityId id, Pos p)
{
    KNpc* e = find_mutable(id);
    if (e == nullptr || !e->alive()) return false;
    p = clamp(p);
    if (cfg_.map) p = cfg_.map->nearest_walkable(p);
    e->set_pos(p);
    Cell from, to;
    if (grid_.move(id, p, from, to)) on_cell_change(*e, from, to);
    emit_move(*e);
    return true;
}

bool KSubWorld::set_ai_mode(EntityId id, int mode)
{
    KNpc* e = find_mutable(id);
    if (e == nullptr || e->kind == KNpcKind::player) return false;
    e->ai_mode = mode;
    e->people_id = EntityId{};
    e->next_ai_time = 0;
    return true;
}

// Per tick: KNpc::OnSpecial1 / OnHurt / OnDeath / OnRevive, then keep swinging at the target.
void KSubWorld::update_action(KNpc& e)
{
    switch (e.doing) {
    case KDoing::attack:
        if (e.wait_for_frame()) {
            e.doing = KDoing::stand;
            if (e.ai_mode != 0) e.attack_target = EntityId{};   // OnSkill: the ai decides again (m_ProcessAI = 1)
        } else if (e.reach_frame(kAttackEffectPercent)) {
            if (e.attack_target == e.id) {
                heal(e);
            } else {
                const auto it = entities_.find(e.attack_target);
                if (it != entities_.end() && it->second.alive()) {
                    // the old melee missile only reaches so far: a target that stepped away is missed
                    const std::int64_t dx = e.pos().x - it->second.pos().x;
                    const std::int64_t dy = e.pos().y - it->second.pos().y;
                    const std::int64_t reach = reach_of(e) + KNpcAI::kMiniAttackRange;
                    if (dx * dx + dy * dy <= reach * reach) {
                        hit(e, it->second);
                    } else {
                        log::trace("zone.fight", "swing missed", {log::kv("attacker", e.id), log::kv("target", it->first)});
                    }
                }
            }
        }
        break;
    case KDoing::hurt:
        if (e.wait_for_frame()) e.doing = KDoing::stand;
        break;
    case KDoing::death:
        if (e.wait_for_frame()) do_revive(e);
        break;
    case KDoing::revive:
        if (e.wait_for_frame()) revive(e);
        break;
    default:
        break;
    }
    if (e.ai_mode == 0 && e.doing == KDoing::stand && e.attack_target.value != 0) {
        const auto it = entities_.find(e.attack_target);
        if (it == entities_.end() || !it->second.alive()) {
            e.attack_target = EntityId{};   // dead or gone
        } else if (in_reach(e, it->second)) {
            if (e.moving) {
                e.set_pos(e.pos());   // arrived within reach: stop and swing
                emit_move(e);
            }
            start_attack(e, it->second);
        } else if (!e.moving) {
            approach(e, it->second);
        }
    }
}

// The swing lands: placeholder damage (the old formula with attack rating, defence and
// resistances comes with the skill system), then KNpc::DoHurt or DoDeath on the target.
void KSubWorld::hit(KNpc& attacker, KNpc& target)
{
    // KNpc::ReceiveDamage: the attack rating check first (闪过攻击 = dodged, nothing else happens)
    if (!check_hit_target(static_cast<int>(attacker.attack_rating), static_cast<int>(target.defend))) {
        log::trace("zone.fight", "dodged", {log::kv("attacker", attacker.id), log::kv("target", target.id)});
        return;
    }
    target.people_id = attacker.id;   // m_nPeopleIdx = nLauncher (passive ais strike back at it)
    // KNpc::CalcDamage(damage_physics): the blow, then the physics resistance (MAX_RESIST); the
    // shields, damage return, mana and the PK rate come with the skill system
    const int min = static_cast<int>(attacker.min_damage);
    const int max = static_cast<int>(attacker.max_damage);
    if (min + max <= 0) return;
    int dmg = max - min < 0 ? max + random(min - max) : min + random(max - min);
    const int res = std::min(target.physics_resist, kMaxResist);
    dmg = dmg * (100 - res) / 100;
    if (dmg <= 0) return;
    // m_CurrentLife -= nDamage; DoDeath only below zero: a blow that leaves exactly 0 leaves it standing
    const bool dies = static_cast<std::uint32_t>(dmg) > target.life;
    target.life = dies ? 0u : target.life - static_cast<std::uint32_t>(dmg);
    log::debug("zone.fight", "hit", {log::kv("attacker", attacker.id), log::kv("target", target.id), log::kv("damage", dmg), log::kv("resist", res), log::kv("life", target.life)});
    emit_life(target, -dmg, attacker.id);
    if (dies) {
        do_death(target, attacker.id);
    } else {
        do_hurt(target, attacker.id);
    }
}

// KNpc::CheckHitTarget: hit chance from the attack rating against the (partly ignored) defence.
bool KSubWorld::check_hit_target(int ar, int df, int ignore)
{
    const int defense = df * (100 - ignore) / 100;
    int percent = (ar + defense) == 0 ? 50 : ar * 100 / (ar + defense);
    percent = std::clamp(percent, kMinHitPercent, kMaxHitPercent);
    return rand_percent(percent);
}

// KNpc::ProcessState every GAME_UPDATE_TIME frames: 生命自然回复 (the life replenish of the level data).
void KSubWorld::process_state(KNpc& e)
{
    if (!e.alive() || e.life_replenish == 0 || e.life >= e.life_max) return;
    const std::int64_t next = std::clamp<std::int64_t>(static_cast<std::int64_t>(e.life) + e.life_replenish, 0, e.life_max);
    const auto delta = static_cast<std::int32_t>(next - static_cast<std::int64_t>(e.life));
    if (delta == 0) return;
    e.life = static_cast<std::uint32_t>(next);
    emit_life(e, delta, EntityId{});
}

const KNpcLevelData& KSubWorld::level_data_of(const KNpcTemplate& t, int level, int series) const
{
    const std::uint64_t key = (static_cast<std::uint64_t>(t.id) << 32) | (static_cast<std::uint64_t>(level & 0xffff) << 8) | static_cast<std::uint64_t>(series & 0xff);
    auto it = level_cache_.find(key);
    if (it == level_cache_.end()) {
        KNpcLevelData d = KNpcTemplateSet::level_data(t, level, series, cfg_.scripts.get());
        log::debug("npc", "level data", {log::kv("template", t.id), log::kv("level", level), log::kv("script", d.from_script),
                                         log::kv("life", d.life_max), log::kv("ar", d.attack_rating), log::kv("defend", d.defend),
                                         log::kv("min_damage", d.min_damage), log::kv("max_damage", d.max_damage), log::kv("exp", d.exp)});
        it = level_cache_.emplace(key, std::move(d)).first;
    }
    return it->second;
}

// A heal cast on oneself (AIMode 2 / 5, skill 1).  Placeholder amount until the skill system
// brings the real formula: a fifth of the maximum.
void KSubWorld::heal(KNpc& e)
{
    if (!e.alive() || e.life >= e.life_max) return;
    const std::uint32_t amount = std::min(std::max(1u, e.life_max / 5), e.life_max - e.life);
    e.life += amount;
    log::debug("zone.fight", "heal", {log::kv("entity", e.id), log::kv("amount", amount), log::kv("life", e.life)});
    emit_life(e, static_cast<std::int32_t>(amount), e.id);
}

// KNpc::DoHurt (server side): HitRecover lowers the chance and the length of the stagger.
void KSubWorld::do_hurt(KNpc& e, EntityId source)
{
    if (e.doing == KDoing::hurt || !e.alive()) return;
    if (e.hit_recover >= 100) return;
    const std::uint32_t chance = kMinHurtPercent + e.hit_recover * (100 - kMinHurtPercent) / 100;
    if (rng_() % 100 >= chance) return;   // g_RandPercent
    e.doing = KDoing::hurt;
    e.frame_total = std::max(1u, e.hurt_frame * (100 - e.hit_recover) / 100);
    e.frame_cur = 0;
    if (e.moving) e.set_pos(e.pos());   // the hit interrupts walking
    emit_action(e, pb::ACTION_HURT, source);
}

// KNpc::DoDeath: players outside fight mode keep 1 life (the old rule); npcs play the death
// animation, then wait ReviveFrame frames out of sight and respawn at home.
void KSubWorld::do_death(KNpc& e, EntityId killer)
{
    if (e.doing == KDoing::death) return;
    if (e.kind == KNpcKind::player) {
        e.life = 1;
        emit_life(e, 0, killer);
        return;
    }
    e.doing = KDoing::death;
    e.frame_total = std::max(1u, e.death_frame);
    e.frame_cur = 0;
    e.attack_target = EntityId{};
    if (e.moving) e.set_pos(e.pos());
    log::debug("zone.fight", "death", {log::kv("entity", e.id), log::kv("killer", killer)});
    emit_action(e, pb::ACTION_DEATH, killer);
}

// KNpc::DoRevive (server): the corpse leaves the region for ReviveFrame frames.
void KSubWorld::do_revive(KNpc& e)
{
    e.doing = KDoing::revive;
    e.frame_total = std::max(1u, e.revive_frame);
    e.frame_cur = 0;
    scratch_sids_.clear();
    viewers_of(grid_.cell_of(e.pos()), scratch_sids_, e.id);
    if (!scratch_sids_.empty()) {
        pb::EntityDespawn gone;
        gone.add_entity_ids(e.id.value);
        emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_DESPAWN), gone);
    }
    grid_.remove(e.id);
}

// KNpc::Revive: back at the home position with full life.
void KSubWorld::revive(KNpc& e)
{
    e.life = e.life_max;
    e.doing = KDoing::stand;
    e.frame_total = 0;
    e.frame_cur = 0;
    // KNpc::RestoreNpcBaseInfo + the ai part of Init
    e.people_id = EntityId{};
    e.attack_target = EntityId{};
    e.ai_add_life_time = 0;
    e.current_camp = e.camp;
    e.active_skill_id = 0;
    e.current_attack_radius = 30;
    e.current_active_radius = e.active_radius;
    e.next_ai_time = 0;
    e.set_pos(e.home);
    grid_.insert(e.id, e.home);
    scratch_sids_.clear();
    viewers_of(grid_.cell_of(e.home), scratch_sids_, e.id);
    if (!scratch_sids_.empty()) {
        pb::EntitySpawn me;
        fill_info(e, *me.add_entities());
        emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), me);
    }
    log::debug("zone.fight", "revived", {log::kv("entity", e.id)});
}

void KSubWorld::broadcast(const KNpc& e, std::uint16_t msg_id, const google::protobuf::MessageLite& msg)
{
    scratch_sids_.clear();
    viewers_of(grid_.cell_of(e.pos()), scratch_sids_, e.id);
    if (e.kind == KNpcKind::player && e.sid != 0) scratch_sids_.push_back(e.sid);
    if (!scratch_sids_.empty()) emit(scratch_sids_, msg_id, msg);
}

void KSubWorld::emit_action(const KNpc& e, pb::Action action, EntityId target)
{
    pb::EntityAction a;
    a.set_entity_id(e.id.value);
    a.set_action(action);
    a.set_target(target.value);
    a.set_dir(e.dir);
    a.set_frames(e.frame_total);
    set_vec(a.mutable_pos(), e.pos());
    a.set_tick(tick_);
    broadcast(e, static_cast<std::uint16_t>(pb::G2C_ENTITY_ACTION), a);
}

void KSubWorld::emit_life(const KNpc& e, std::int32_t delta, EntityId source)
{
    pb::EntityLife l;
    l.set_entity_id(e.id.value);
    l.set_life(e.life);
    l.set_life_max(e.life_max);
    l.set_delta(delta);
    l.set_source(source.value);
    broadcast(e, static_cast<std::uint16_t>(pb::G2C_ENTITY_LIFE), l);
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
