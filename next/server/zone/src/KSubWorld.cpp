#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <fmt/format.h>

#include "jx/core/Metrics.h"

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KNpcAI.h"
#include "jx/zone/ScriptFuns.h"

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

// How many cells of `cell` it takes to cover half of `extent`, rounded up.  Rounding up matters:
// the view must never be smaller than the screen, or an entity the player can see was never sent.
std::int32_t view_cells_for(std::int32_t extent, std::int32_t cell)
{
    if (cell <= 0) return 1;
    const std::int32_t half = std::max(0, extent) / 2;
    return std::max(1, (half + cell - 1) / cell);
}

// The interest grid of a world: a rectangle in scene units turned into cell counts.  A test may
// pin the old square behaviour by setting view_cells.
KRegionGrid make_grid(const KSubWorldConfig& cfg)
{
    if (cfg.view_cells > 0) return KRegionGrid(cfg.cell_size, cfg.view_cells, cfg.view_cells);
    return KRegionGrid(cfg.cell_size,
                       view_cells_for(cfg.view_width, cfg.cell_size),
                       view_cells_for(cfg.view_height, cfg.cell_size));
}

} // namespace

KSubWorld::KSubWorld(KSubWorldConfig cfg)
    : cfg_(std::move(cfg)), grid_(make_grid(cfg_)), rng_(cfg_.seed)
{
    paths_.reset(cfg_.map.get());
    profile_ = std::make_unique<core::TickProfile>(
        core::metrics(), fmt::format("map.{}", cfg_.map ? static_cast<std::uint32_t>(cfg_.map->id) : 0u));
    if (cfg_.tick_hz == 0) cfg_.tick_hz = 20;
    if (cfg_.map) {
        cfg_.width = cfg_.map->scene_w;
        cfg_.height = cfg_.map->scene_h;
        if (!cfg_.spawn_from_config) cfg_.spawn_point = cfg_.map->spawn;
        cfg_.spawn_point = cfg_.map->nearest_walkable(clamp(cfg_.spawn_point));
        grid_ = make_grid(cfg_);
        if (cfg_.map_npcs) {
            for (const KNpcPlacement& n : cfg_.map->npcs) {
                // NPCKIND of the old GameDataDef.h: 0 = kind_normal (a monster); everything else
                // (partner, dialoger, bird, mouse) is a friendly npc
                const KNpcKind kind = n.kind == 0 ? KNpcKind::monster : KNpcKind::npc;
                const EntityId id = spawn_npc(n.name, n.pos, n.template_id, 0, kind);
                if (KNpc* placed = entities_.find(id)) {
                    placed->dir = static_cast<std::uint32_t>(n.dir & 63);
                    placed->level = static_cast<std::uint32_t>(std::max(1, n.level));
                    placed->series = static_cast<std::uint32_t>(n.series);
                    apply_template(*placed);   // skill levels depend on the level (KNpcTemplate::InitNpcLevelData)
                    placed->npc_kind = n.kind;   // KNpcSet::Add: m_Kind / m_Camp come from the placement (KSNpcInfo)
                    placed->camp = std::clamp(n.camp, 0, camp_num - 1);
                    placed->current_camp = placed->camp;
                }
            }
            take_outbox();   // nobody is listening yet
            log::info("zone", "map npcs placed", {log::kv("count", cfg_.map->npcs.size())});
        }
    }
}

Pos KSubWorld::to_local(Pos absolute) const noexcept
{
    const Pos o = cfg_.map ? cfg_.map->origin : Pos{};
    return Pos{absolute.x - o.x, absolute.y - o.y};
}

Pos KSubWorld::to_absolute(Pos local) const noexcept
{
    const Pos o = cfg_.map ? cfg_.map->origin : Pos{};
    return Pos{local.x + o.x, local.y + o.y};
}

Pos KSubWorld::clamp(Pos p) const noexcept
{
    return Pos{std::clamp(p.x, 0, cfg_.width - 1), std::clamp(p.y, 0, cfg_.height - 1)};
}

const KNpc* KSubWorld::find_entity(EntityId id) const
{
    return entities_.find(id);
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

void KSubWorld::emit_spawn(const std::vector<std::uint64_t>& sids, const pb::EntitySpawn& spawn)
{
    if (sids.empty() || spawn.entities_size() == 0) return;
    if (spawn.entities_size() <= kSpawnChunk) {
        emit(sids, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), spawn);
        return;
    }
    pb::EntitySpawn chunk;
    for (int i = 0; i < spawn.entities_size(); ++i) {
        *chunk.add_entities() = spawn.entities(i);
        if (chunk.entities_size() == kSpawnChunk) {
            emit(sids, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), chunk);
            chunk.clear_entities();
        }
    }
    if (chunk.entities_size() > 0) emit(sids, static_cast<std::uint16_t>(pb::G2C_ENTITY_SPAWN), chunk);
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
    if (e.kind == KNpcKind::drop) {
        if (e.object.kind == KObjKind::money) {
            out.set_count(static_cast<std::uint32_t>(e.object.money));
        } else if (const KItem* it = ground_item(e.id)) {
            out.set_count(static_cast<std::uint32_t>(it->count));
        }
    }
}

pb::Result KSubWorld::spawn_player(std::uint64_t sid, const pb::RoleData& role, EntityId& entity_out, Pos& pos_out, const Pos* at)
{
    if (sid == 0) return pb::RESULT_BAD_REQUEST;
    if (players_.contains(sid)) return pb::RESULT_WRONG_STATE;
    if (players_.size() >= cfg_.max_players) return pb::RESULT_FULL;

    KNpc e;
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
    e.fight_mode = role.fight_mode();
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
    if (at != nullptr) {
        start = clamp(*at);
    } else if (role.has_position() && role.position().zone_id() == cfg_.zone_id && role.position().has_pos() &&
               (role.position().map_id() == 0 || role.position().map_id() == map_id())) {
        start = clamp(Pos{role.position().pos().x(), role.position().pos().y()});
    }
    if (cfg_.map) start = cfg_.map->nearest_walkable(start);
    e.set_pos(start);
    const EntityId id = entities_.insert(std::move(e));   // the table owns the handle (SPEC 36)
    entities_.at(id).id = id;
    grid_.insert(id, start, true);   // a player keeps its neighbourhood awake (SPEC 44, 45)
    players_[sid] = id;
    roles_[sid] = role;
    load_items(sid, role);

    // The newcomer looks around at once: its client gets its own character first, then what is
    // nearest, up to the budget; the rest follows over the next ticks.  Everybody already there
    // notices the newcomer at their own next look (KInterest.cpp) - nobody is told in a burst.
    KViewer& v = viewers_[sid];
    v = KViewer{};
    v.self = id;
    look_around(sid, v);
    // routine looks of players that arrive together are spread over the period, not all on one tick
    v.next_look = tick_ + 1 + sid % std::max<std::uint32_t>(1, cfg_.interest_period);
    // then what the character carries (s2c_syncitem of the old server followed the player sync)
    send_item_list(sid);

    entity_out = id;
    pos_out = start;
    log::ScopedContext ctx(log::Context{sid, role.player_id(), cfg_.zone_id, tick_});
    log::info("zone", "player spawned", {log::kv("entity", id), log::kv("name", role.name()), log::kv("x", start.x), log::kv("y", start.y),
                                         log::kv("visible", v.known.size())});
    return pb::RESULT_OK;
}

bool KSubWorld::remove_player(std::uint64_t sid)
{
    const auto pit = players_.find(sid);
    if (pit == players_.end()) return false;
    const EntityId id = pit->second;
    drop_viewer(sid);   // first: a client that is leaving is not told about its own departure
    if (KNpc* gone_e = entities_.find(id)) {
        const std::size_t told = gone_e->watchers.size();
        entity_gone(*gone_e);   // exactly the clients that know this player, however crowded the place
        log::ScopedContext ctx(log::Context{sid, gone_e->player_id, cfg_.zone_id, tick_});
        log::info("zone", "player removed", {log::kv("entity", id), log::kv("x", gone_e->pos().x), log::kv("y", gone_e->pos().y),
                                             log::kv("told", told)});
        grid_.remove(id);
        entities_.destroy(id);
    }
    players_.erase(pit);
    roles_.erase(sid);
    items_.erase(sid);
    return true;
}

void KSubWorld::fill_move(const KNpc& e, pb::EntityMove& mv) const
{
    mv.set_entity_id(e.id.value);
    set_vec(mv.mutable_pos(), e.pos());
    set_vec(mv.mutable_target(), e.moving ? e.destination() : e.pos());
    mv.set_move_speed(e.speed);
    mv.set_tick(tick_);
    if (e.moving) {
        set_vec(mv.add_path(), e.target());
        for (const Pos& p : e.path) set_vec(mv.add_path(), p);
    }
}

void KSubWorld::emit_move(const KNpc& e)
{
    if (e.watchers.empty()) return;   // nobody knows it: a npc wandering on an empty map costs nothing
    pb::EntityMove mv;
    fill_move(e, mv);

    // Everybody that knows it - at once when the mover is near the watcher, gathered into that
    // watcher's next EntityMoves when it is far (N3, flush_far).  The player's own client always
    // gets its copy at once, with the sequence number of the request it answers.
    const bool split = cfg_.far_period > 1;
    const std::int64_t near2 = near_radius2();
    const Pos at = e.pos();
    scratch_sids_.clear();
    bool to_self = false;
    for (const std::uint64_t sid : e.watchers) {
        if (e.sid != 0 && sid == e.sid) {
            to_self = true;
            continue;
        }
        if (split) {
            const auto it = viewers_.find(sid);
            if (it != viewers_.end()) {
                KViewer& v = it->second;
                const KNpc* w = entities_.find(v.self);
                if (w != nullptr) {
                    const std::int64_t dx = static_cast<std::int64_t>(w->pos().x) - at.x;
                    const std::int64_t dy = static_cast<std::int64_t>(w->pos().y) - at.y;
                    if (dx * dx + dy * dy > near2) {
                        const auto pos = std::lower_bound(v.far_pending.begin(), v.far_pending.end(), e.id,
                                                          [](EntityId a, EntityId b) { return a.value < b.value; });
                        if (pos == v.far_pending.end() || *pos != e.id) v.far_pending.insert(pos, e.id);
                        continue;
                    }
                }
            }
        }
        scratch_sids_.push_back(sid);
    }
    if (!scratch_sids_.empty()) {
        mv.set_seq(0);
        emit(scratch_sids_, static_cast<std::uint16_t>(pb::G2C_ENTITY_MOVE), mv);
    }
    if (to_self) {
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
        std::vector<Pos> path = paths_.find(e.pos(), dest);
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
    if (cfg_.gm_chat && gm_command(sid, text)) return true;   // TextGMFilter runs before the text is spoken
    const KNpc& e = entities_.at(pit->second);
    pb::ChatMsg msg;
    msg.set_entity_id(e.id.value);
    msg.set_name(e.name);
    msg.set_text(std::string(text));
    // Nearby chat is heard by whoever SEES the speaker: the clients that know the character,
    // which includes the speaker's own.
    emit(e.watchers, static_cast<std::uint16_t>(pb::G2C_CHAT_MSG), msg);
    log::ScopedContext ctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    log::debug("zone.chat", "chat", {log::kv("entity", e.id), log::kv("len", text.size()), log::kv("receivers", e.watchers.size())});
    return true;
}

// KGMCommand.cpp: "?gm <command> <text>" - `ds` (DoSct) runs the text as the player's script
// action, `dw` runs it as a world script (no player).  Anything else is not a command.
bool KSubWorld::gm_command(std::uint64_t sid, std::string_view text)
{
    if (text.size() < 5 || (text.substr(0, 4) != "?gm " && text.substr(0, 4) != "?GM ")) return false;
    std::string_view rest = text.substr(4);
    const auto space = rest.find(' ');
    const std::string_view cmd = rest.substr(0, space);
    const std::string_view param = space == std::string_view::npos ? std::string_view{} : rest.substr(space + 1);
    const bool for_player = cmd == "ds" || cmd == "DoSct";
    const bool for_world = cmd == "dw";
    if (!for_player && !for_world) {
        msg_to_player(sid, "GM: unknown command");
        return true;
    }
    if (param.empty() || param.size() >= 300) {   // szScriptAction[300]
        msg_to_player(sid, "GM: nothing to run");
        return true;
    }
    if (!gm_script_) {
        gm_script_ = std::make_unique<KLuaScript>();
        if (!gm_script_->init(cfg_.scripts ? cfg_.scripts->roots().empty() ? "" : cfg_.scripts->roots().front() : "")) {
            gm_script_.reset();
            msg_to_player(sid, "GM: no script state");
            return true;
        }
    }
    KNpc* me = find_mutable(players_.at(sid));
    KScriptContext& ctx = g_ScriptContext();
    const KScriptContext saved = ctx;
    ctx.world = this;
    ctx.player = for_player ? me : nullptr;
    ctx.sid = for_player ? sid : 0;
    std::string error;
    const bool ok = gm_script_->do_string(std::string(param), for_player ? "gm ds" : "gm dw", &error);
    ctx = saved;
    log::ScopedContext lctx(log::Context{sid, me ? me->player_id : 0, cfg_.zone_id, tick_});
    log::info("zone.gm", "gm command", {log::kv("command", std::string(cmd)), log::kv("code", std::string(param)), log::kv("ok", ok)});
    msg_to_player(sid, ok ? "GM: ok" : "GM: " + error);
    return true;
}

EntityId KSubWorld::spawn_npc(std::string name, Pos pos, std::uint32_t template_id, std::int32_t wander_radius, KNpcKind kind)
{
    KNpc e;
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
    const Pos home = e.home;
    max_vision_ = std::max(max_vision_, e.vision_radius);
    const EntityId id = entities_.insert(std::move(e));
    entities_.at(id).id = id;
    grid_.insert(id, home);
    // Nothing is sent here: the clients around notice the newcomer at their next look, which also
    // means a script that spawns a hundred npcs at once does not flood anybody.
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
        e.set_path(paths_.find(e.pos(), dest, 2000));
    } else {
        e.set_target(dest);
    }
    e.next_wander_tick = tick_ + cfg_.tick_hz * 2 + static_cast<std::uint64_t>(rng_() % (cfg_.tick_hz * 6));
    if (e.moving) emit_move(e);
}

void KSubWorld::tick()
{
    ++tick_;
    // MASTER SPEC 44/45 taken to the whole map: with nobody on it and nothing left awake, there is
    // no observable state to advance, so the tick returns before it even lists the entities.  A
    // zone hosting 980 maps otherwise walks 110 730 npcs 18 times a second to skip every one.
    // The A* buffers (13 bytes per cell, 11 MB on Phượng Tường) go back to the allocator too.
    if (players_.empty() && awake_entities_ == 0) {
        if (++idle_ticks_ > kDormantAfterTicks) {
            if (!dormant_) {
                dormant_ = true;
                paths_.release();
            }
            return;
        }
    } else {
        idle_ticks_ = 0;
        dormant_ = false;
    }

    // deterministic iteration order regardless of hash layout
    entities_.ids(scratch_ids_);
    std::sort(scratch_ids_.begin(), scratch_ids_.end());

    {
        auto phase = profile_->phase(core::TickPhase::spatial_update);
        build_awake_cells();
    }
    awake_entities_ = 0;

    auto ai_phase = std::make_unique<core::ScopedTiming>(profile_->phase_timing(core::TickPhase::ai));
    std::vector<EntityId> moved, arrived, expired_objects;
    for (const EntityId id : scratch_ids_) {
        KNpc& e = entities_.at(id);
        // MASTER SPEC 44 / 45: a npc nobody can see does not think.  It keeps its state (and its
        // regeneration), but the ai and the wandering - the expensive parts - are skipped until a
        // player comes near again.  A npc that is busy (moving, fighting, hurt, dead) stays awake
        // so nothing can freeze half way through an action.
        if (e.kind == KNpcKind::drop) {   // KObj::Activate: no ai, no moving - a countdown
            object_tick(e, expired_objects);
            if (!e.object.forever) ++awake_entities_;   // a map with things on the ground keeps ticking until they are gone
            continue;
        }
        const bool awake = is_awake(e);
        if (awake) ++awake_entities_;
        // KNpc::Activate: m_LoopFrames++, ProcessState every GAME_UPDATE_TIME frames, then NpcAI.Activate
        // while m_ProcessAI, then the command / status of the frame
        ++e.loop_frames;
        const std::uint64_t state_every = awake ? kGameUpdateTime : kGameUpdateTime * 8;   // dormant: rarely
        if (e.loop_frames % state_every == 0) process_state(e);
        if (e.life_state.time > 0 && e.alive()) process_potions(e);   // ProcessState runs only while m_ProcessState (cleared by DoDeath)
        // KNpcAI::ProcessPlayer -> TriggerMapTrap -> KNpc::CheckTrap (players, while m_ProcessAI)
        if (e.kind == KNpcKind::player && e.process_ai()) check_trap(e);
        if (awake && e.kind != KNpcKind::player && e.ai_mode != 0 && e.process_ai()) KNpcAI::activate(*this, e);
        update_action(e);
        if (awake && e.kind != KNpcKind::player && e.ai_mode == 0 && e.wander_radius > 0 && !e.moving && e.doing == KDoing::stand && tick_ >= e.next_wander_tick) wander(e);
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
    ai_phase.reset();
    for (const EntityId id : expired_objects) remove_object(id);
    flush_pending_drops();   // ai + movement integration end here

    {
        // spatial: re-file whatever crossed a cell.  A player that did sees another part of the
        // map now, so its client looks around this very tick; everybody else notices the mover at
        // their own next look.  Then every client that is due looks around (KInterest.cpp).
        auto phase = profile_->phase(core::TickPhase::interest);
        for (const EntityId id : moved) {
            KNpc& e = entities_.at(id);
            Cell from, to;
            if (!grid_.move(id, e.pos(), from, to)) continue;
            if (e.kind != KNpcKind::player || e.sid == 0) continue;
            if (const auto v = viewers_.find(e.sid); v != viewers_.end()) v->second.dirty = true;
        }
        run_interest();
    }
    {
        auto phase = profile_->phase(core::TickPhase::snapshot);
        for (const EntityId id : arrived) emit_move(entities_.at(id));
        flush_far();
    }
}

// The cells that hold a player, grown by the largest vision radius in this map: everything
// inside stays awake, everything outside sleeps (MASTER SPEC 44, 45).
void KSubWorld::build_awake_cells()
{
    awake_cells_.clear();
    const std::int32_t reach = awake_radius_cells();
    grid_.for_each_player_cell([&](Cell c) {
        for (std::int32_t cy = c.cy - reach; cy <= c.cy + reach; ++cy) {
            for (std::int32_t cx = c.cx - reach; cx <= c.cx + reach; ++cx) {
                awake_cells_.insert((static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) |
                                    static_cast<std::uint64_t>(static_cast<std::uint32_t>(cy)));
            }
        }
    });
}

std::int32_t KSubWorld::awake_radius_cells() const noexcept
{
    const std::int32_t by_vision = (max_vision_ + cfg_.cell_size - 1) / std::max(1, cfg_.cell_size);
    return std::max(cfg_.view_cells + 1, by_vision);
}

bool KSubWorld::is_awake(const KNpc& e) const
{
    if (e.kind == KNpcKind::player) return true;
    // busy npcs never fall asleep mid action
    if (e.moving || e.doing != KDoing::stand || e.attack_target.value != 0) return true;
    // nor does one that still has to walk back home (KNpcAI::KeepActiveRange)
    const int radius = e.current_active_radius > 0 ? e.current_active_radius : e.active_radius;
    if (radius > 0) {
        const std::int64_t dx = e.pos().x - e.home.x;
        const std::int64_t dy = e.pos().y - e.home.y;
        if (dx * dx + dy * dy > static_cast<std::int64_t>(radius) * radius) return true;
    }
    if (awake_cells_.empty()) return false;
    const Cell c = grid_.cell_of(e.pos());
    return awake_cells_.contains((static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.cx)) << 32) |
                                 static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.cy)));
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
    e.treasure = t->treasure;
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
    KNpc* t = entities_.find(target);
    if (t == nullptr || target == e.id || !t->alive()) return false;
    if (t->kind == KNpcKind::npc || t->kind == KNpcKind::drop) return false;   // townsfolk cannot be attacked
    log::ScopedContext ctx(log::Context{sid, e.player_id, cfg_.zone_id, tick_});
    e.move_seq = seq;
    e.attack_target = target;
    e.approach_tries = 0;
    if (in_reach(e, *t)) {
        if (e.moving) {
            e.set_pos(e.pos());
            emit_move(e);
        }
        if (e.doing == KDoing::stand) start_attack(e, *t);
    } else {
        // like the old client: walk up to the target first, the swing starts on arrival
        approach(e, *t);
    }
    log::trace("zone.fight", "attack request", {log::kv("entity", e.id), log::kv("target", target), log::kv("seq", seq),
                                                 log::kv("in_reach", in_reach(e, *t))});
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
        std::vector<Pos> path = paths_.find(e.pos(), dest);
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
        std::vector<Pos> path = paths_.find(e.pos(), dest, 4000);
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
    return entities_.find(id);
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
    if (grid_.move(id, p, from, to)) {
        // A jump to another part of the map: whoever knew it here is told it is gone (they did not
        // see it walk away, so a move would make it slide across their screen), and the clients
        // around the landing place find it at their next look.  Its own client keeps its character
        // and looks around the new place this tick.
        entity_gone(*e, true);
        if (e->sid != 0) {
            if (const auto v = viewers_.find(e->sid); v != viewers_.end()) v->second.dirty = true;
        }
    }
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
                KNpc* t = entities_.find(e.attack_target);
                if (t != nullptr && t->alive()) {
                    // the old melee missile only reaches so far: a target that stepped away is missed
                    const std::int64_t dx = e.pos().x - t->pos().x;
                    const std::int64_t dy = e.pos().y - t->pos().y;
                    const std::int64_t reach = reach_of(e) + KNpcAI::kMiniAttackRange;
                    if (dx * dx + dy * dy <= reach * reach) {
                        hit(e, *t);
                    } else {
                        log::trace("zone.fight", "swing missed", {log::kv("attacker", e.id), log::kv("target", e.attack_target)});
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
        KNpc* t = entities_.find(e.attack_target);
        if (t == nullptr || !t->alive()) {
            e.attack_target = EntityId{};   // dead or gone
        } else if (in_reach(e, *t)) {
            if (e.moving) {
                e.set_pos(e.pos());   // arrived within reach: stop and swing
                emit_move(e);
            }
            start_attack(e, *t);
        } else if (!e.moving) {
            approach(e, *t);
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
    // jx_linux_y 0x0808B65F: m_CurrentLife += replenish * percent / 100, then both clamps
    const std::int64_t gain = static_cast<std::int64_t>(e.life_replenish) * e.life_replenish_percent / 100;
    const std::int64_t next = std::clamp<std::int64_t>(static_cast<std::int64_t>(e.life) + gain, 0, e.life_max);
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
    lose_treasure(e, killer);
}

// KNpc::DoRevive (server): the corpse leaves the region for ReviveFrame frames.
void KSubWorld::do_revive(KNpc& e)
{
    e.doing = KDoing::revive;
    e.life_state = {};   // KNpc::DoRevive -> ClearNormalState
    e.frame_total = std::max(1u, e.revive_frame);
    e.frame_cur = 0;
    entity_gone(e);   // every client that saw it die sees the corpse go
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
    grid_.insert(e.id, e.home);   // back in the world: the clients around find it at their next look
    log::debug("zone.fight", "revived", {log::kv("entity", e.id)});
}

// KNpc::CheckTrap: the trap under the player fires once when stepped on (m_TrapScriptID keeps
// the current one; leaving it resets, so the same trap can fire again later).
void KSubWorld::check_trap(KNpc& e)
{
    if (!cfg_.map) return;
    const std::uint32_t id = cfg_.map->trap_at(e.pos());
    if (e.trap_script_id == id) return;
    e.trap_script_id = id;
    if (id == 0) return;
    const std::string& script = cfg_.map->trap_script(id);
    if (script.empty() || !cfg_.scripts) {
        if (!trap_warned_[id]) {
            trap_warned_[id] = true;
            log::warn("zone.trap", "trap without a script", {log::kv("map", map_id()), log::kv("trap", id), log::kv("x", e.pos().x), log::kv("y", e.pos().y),
                                                              log::kv("scripts", static_cast<bool>(cfg_.scripts))});
        }
        return;
    }
    log::debug("zone.trap", "trap", {log::kv("entity", e.id), log::kv("trap", id), log::kv("script", script)});
    execute_script(script, "main", e, 0);   // Player.ExecuteScript(m_TrapScriptID, "main", 0)
}

bool KSubWorld::execute_script(const std::string& game_path, const char* fn, KNpc& player, int param)
{
    if (!cfg_.scripts) return false;
    KLuaScript* script = cfg_.scripts->get(game_path);
    if (script == nullptr) return false;
    KScriptContext& ctx = g_ScriptContext();
    const KScriptContext saved = ctx;
    ctx.world = this;
    ctx.player = &player;
    ctx.sid = player.sid;
    const bool ok = script->call_number(fn, {static_cast<double>(param)}).has_value() || script->has_function(fn);
    ctx = saved;
    if (!ok) log::warn("zone.trap", "script function missing", {log::kv("script", game_path), log::kv("function", fn)});
    return ok;
}

bool KSubWorld::set_pos(EntityId id, Pos p)
{
    if (!teleport(id, p)) return false;
    KNpc* e = find_mutable(id);
    if (e->doing == KDoing::attack) do_stand(*e);   // DoStand(); m_ProcessAI = 1
    return true;
}

int KSubWorld::change_world_request(KNpc& player, std::uint32_t target_map, Pos pos)
{
    if (player.kind != KNpcKind::player) return 0;
    if (target_map == map_id()) return set_pos(player.id, to_local(pos)) ? 1 : 0;   // 切换的世界就是本身: 只需切换座标
    log::info("zone.trap", "world change requested", {log::kv("entity", player.id), log::kv("from", map_id()), log::kv("to", target_map),
                                                       log::kv("x", pos.x), log::kv("y", pos.y)});
    world_changes_.push_back(KWorldChange{player.sid, target_map, pos});   // absolute Mps: the target converts
    return 1;
}

std::vector<KWorldChange> KSubWorld::take_world_changes()
{
    std::vector<KWorldChange> out;
    out.swap(world_changes_);
    return out;
}

void KSubWorld::msg_to_player(std::uint64_t sid, std::string_view text)
{
    if (!players_.contains(sid)) return;
    pb::ChatMsg msg;
    msg.set_text(std::string(text));
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_CHAT_MSG), msg);
}

// To every client that knows the entity - its own included, which is in the list like any other.
void KSubWorld::broadcast(const KNpc& e, std::uint16_t msg_id, const google::protobuf::MessageLite& msg)
{
    if (!e.watchers.empty()) emit(e.watchers, msg_id, msg);
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
        out.mutable_position()->set_map_id(map_id());
        set_vec(out.mutable_position()->mutable_pos(), e->pos());
        out.set_level(e->level);
        out.set_fight_mode(e->fight_mode);   // KNpc::SetFightMode survives a logout like in the old game
    }
    save_items(sid, out);
    return true;
}

// ---- items ----------------------------------------------------------------------------------

KItemList* KSubWorld::items_of(std::uint64_t sid)
{
    const auto it = items_.find(sid);
    return it == items_.end() ? nullptr : &it->second;
}

const KItemList* KSubWorld::items_of(std::uint64_t sid) const
{
    const auto it = items_.find(sid);
    return it == items_.end() ? nullptr : &it->second;
}

std::optional<KItemGenerator> KSubWorld::item_generator(std::uint32_t version)
{
    if (!cfg_.items) return std::nullopt;
    if (version == 0 || cfg_.items->set(version) == nullptr) version = cfg_.items->default_version();
    const KItemTemplateSet* set = cfg_.items->set(version);
    if (set == nullptr) return std::nullopt;
    return KItemGenerator(*set, version, static_cast<std::uint32_t>(rng_()));
}

void KSubWorld::load_items(std::uint64_t sid, const pb::RoleData& role)
{
    KItemList& list = items_[sid];
    list.clear();
    list.set_money(room_equipment, static_cast<int>(role.money()));
    list.set_money(room_repository, static_cast<int>(role.bank_money()));
    int lost = 0;
    for (const auto& data : role.items()) {
        KItem item;
        if (!cfg_.items || !KItem::from_proto(data, *cfg_.items, item)) {
            ++lost;
            continue;
        }
        bool ok = false;
        if (data.room() == room_body) {
            // worn: straight onto the part it was on, no requirement check - it passed when it went on
            const int part = static_cast<int>(data.x());
            if (part >= 0 && part < itempart_num && list.equipped(part) == 0 && KItemList::fits(item.detail, part)) {
                const std::uint32_t id = list.add(item, room_equipment);   // through the bag, then onto the body
                ok = id != 0 && list.wear(id, part);
                if (id != 0 && !ok) list.remove(id);
            }
        } else {
            ok = list.add(item, static_cast<int>(data.room()), static_cast<int>(data.x()), static_cast<int>(data.y())) != 0;
            if (!ok) ok = list.add(item, room_equipment) != 0;   // a cell taken (a table changed sizes): anywhere in the bag
        }
        if (!ok) ++lost;
    }
    if (role.next_item_id() > list.next_id()) list.set_next_id(role.next_item_id());
    if (lost > 0) {
        log::ScopedContext ctx(log::Context{sid, role.player_id(), cfg_.zone_id, tick_});
        log::warn("zone", "items not restored", {log::kv("lost", lost), log::kv("kept", list.size())});
    }
}

// ---- item requests ------------------------------------------------------------------------

void KSubWorld::fill_item_view(const KItem& item, const KItemPlace& place, pb::ItemView& out) const
{
    out.set_id(item.id);
    out.set_genre(static_cast<std::uint32_t>(item.genre));
    out.set_detail(static_cast<std::uint32_t>(item.detail));
    out.set_particular(static_cast<std::uint32_t>(item.particular));
    out.set_level(static_cast<std::uint32_t>(item.level));
    out.set_series(item.series);
    out.set_count(static_cast<std::uint32_t>(item.count));
    out.set_durability(item.durability);
    out.set_max_durability(item.max_durability());
    out.set_ex_type(static_cast<std::uint32_t>(item.ex_type));
    out.set_room(static_cast<std::uint32_t>(place.room));
    out.set_x(static_cast<std::uint32_t>(place.x));
    out.set_y(static_cast<std::uint32_t>(place.y));
    out.set_width(static_cast<std::uint32_t>(item.width()));
    out.set_height(static_cast<std::uint32_t>(item.height()));
    out.set_name(item.name());
    if (item.tpl != nullptr) {
        out.set_image(item.tpl->image);
        out.set_intro(item.tpl->intro);
    }
    out.set_price(static_cast<std::uint32_t>(item.price()));
    const auto magic_out = [](const KMagicAttrib& a, pb::ItemMagic* m) {
        m->set_type(static_cast<std::uint32_t>(a.type));
        for (const int v : a.value) m->add_value(v);
    };
    for (const auto& a : item.base) if (!a.empty()) magic_out(a, out.add_base());
    for (const auto& a : item.require) if (!a.empty()) magic_out(a, out.add_require());
    // all six magic slots, empty ones included: the client tells prefixes (even slots) from
    // suffixes (odd slots) by the index, as KItem::GetDesc of the 2.0 client does
    for (const auto& a : item.magic) magic_out(a, out.add_magic());
    out.set_version(item.version);
    out.set_gen_param(static_cast<std::uint32_t>(item.gen_param));
}

void KSubWorld::send_item_list(std::uint64_t sid)
{
    const KItemList* list = items_of(sid);
    if (list == nullptr) return;
    pb::InventorySync msg;
    list->each([&](const KItem& item, const KItemPlace& place) { fill_item_view(item, place, *msg.add_items()); });
    msg.set_money(static_cast<std::uint32_t>(list->money(room_equipment)));
    msg.set_bank_money(static_cast<std::uint32_t>(list->money(room_repository)));
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_LIST), msg);
}

void KSubWorld::item_result(std::uint64_t sid, std::uint32_t seq, pb::Result result)
{
    pb::ItemResult r;
    r.set_seq(seq);
    r.set_result(result);
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_RESULT), r);
}

void KSubWorld::item_moved(std::uint64_t sid, std::uint32_t id, std::uint32_t seq)
{
    const KItemList* list = items_of(sid);
    const auto place = list ? list->place_of(id) : std::nullopt;
    if (!place) return;
    pb::ItemMove m;
    m.set_id(id);
    m.set_room(static_cast<std::uint32_t>(place->room));
    m.set_x(static_cast<std::uint32_t>(place->x));
    m.set_y(static_cast<std::uint32_t>(place->y));
    m.set_seq(seq);
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_MOVE), m);
}

void KSubWorld::item_changed(std::uint64_t sid, std::uint32_t id)
{
    const KItemList* list = items_of(sid);
    const KItem* item = list ? list->find(id) : nullptr;
    if (item == nullptr) return;
    pb::ItemAdd add;
    fill_item_view(*item, *list->place_of(id), *add.mutable_item());
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_ADD), add);
}

void KSubWorld::send_money(std::uint64_t sid)
{
    const KItemList* list = items_of(sid);
    if (list == nullptr) return;
    pb::MoneySync m;
    m.set_money(static_cast<std::uint32_t>(list->money(room_equipment)));
    m.set_bank_money(static_cast<std::uint32_t>(list->money(room_repository)));
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_MONEY), m);
}

std::function<int(int)> KSubWorld::attrib_of(const KNpc& e) const
{
    // KItemList::EnoughAttrib read the player's strength / dexterity / vitality / energy, the
    // npc's level, series and sex, the faction.  The attribute points are not in the world yet
    // (M12): they read as "enough", the rest is real.
    const int level = static_cast<int>(e.level), series = static_cast<int>(e.series), sex = static_cast<int>(e.sex);
    return [level, series, sex](int type) {
        switch (type) {
        case magic_requirelevel: return level;
        case magic_requireseries: return series;
        case magic_requiresex: return sex;
        case magic_requiremenpai: return 0;
        default: return 1 << 20;
        }
    };
}

bool KSubWorld::item_move_request(std::uint64_t sid, std::uint32_t id, int room, int x, int y, std::uint32_t seq)
{
    KItemList* list = items_of(sid);
    const KItem* item = list ? list->find(id) : nullptr;
    if (item == nullptr) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    if (room < 0 || room >= room_num || room == room_trade) {   // the trade box waits for the trade (KItemList::ExchangeItem: only while trading)
        item_result(sid, seq, pb::RESULT_BAD_REQUEST);
        return false;
    }
    // the quick slots take medicine and town portals only, one of each detail type
    // (ExchangeItem pos_immediacy: CheckSameDetailType)
    if (room == room_immediacy) {
        if (item->genre != KItemGenre::medicine && item->genre != KItemGenre::town_portal) {
            item_result(sid, seq, pb::RESULT_BAD_REQUEST);
            return false;
        }
        if (list->same_detail_in(room_immediacy, item->genre, item->detail, id) != 0) {
            item_result(sid, seq, pb::RESULT_ALREADY_EXISTS);
            return false;
        }
    }
    std::uint32_t displaced = 0;
    if (!list->exchange(id, room, x, y, &displaced)) {
        item_result(sid, seq, pb::RESULT_FULL);
        return false;
    }
    item_moved(sid, id, seq);
    if (displaced != 0) item_moved(sid, displaced, 0);
    return true;
}

bool KSubWorld::item_equip_request(std::uint64_t sid, std::uint32_t id, int part, std::uint32_t seq)
{
    KItemList* list = items_of(sid);
    const KNpc* me = find_player(sid);
    const KItem* item = list ? list->find(id) : nullptr;
    if (item == nullptr || me == nullptr) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    if (part < 0) part = KItemList::equip_place(item->detail);   // KItemList::Equip(nIdx, -1): the part its kind goes to
    const std::uint32_t worn = list->equipped(part);
    if (!list->equip(id, part, attrib_of(*me))) {
        item_result(sid, seq, list->can_equip(*item, part, attrib_of(*me)) ? pb::RESULT_FULL : pb::RESULT_BAD_REQUEST);
        return false;
    }
    item_moved(sid, id, seq);
    if (worn != 0) item_moved(sid, worn, 0);
    return true;
}

bool KSubWorld::item_unequip_request(std::uint64_t sid, int part, std::uint32_t seq)
{
    KItemList* list = items_of(sid);
    const std::uint32_t id = list ? list->equipped(part) : 0;
    if (id == 0) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    if (!list->unequip(part)) {
        item_result(sid, seq, pb::RESULT_FULL);   // no room in the bag
        return false;
    }
    item_moved(sid, id, seq);
    return true;
}

// KItemList::EatMecidine as the Linux server has it (jx_linux_y 0x08204710): a dead player eats
// nothing; a medicine is refused while forbit_takemedicine is set, counts towards the potion
// counter, applies its attributes to the npc (KItem::ApplyMagicAttribToNPC), then one is taken
// off the stack when the row is stackable, else the item goes.  Town portals and script items
// come with their systems (M11 E).
bool KSubWorld::item_use_request(std::uint64_t sid, std::uint32_t id, std::uint32_t seq)
{
    KItemList* list = items_of(sid);
    KItem* item = list ? list->find_mutable(id) : nullptr;
    const auto pit = players_.find(sid);
    KNpc* me = pit == players_.end() ? nullptr : find_mutable(pit->second);
    if (item == nullptr || me == nullptr) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    if (!me->alive() || item->genre != KItemGenre::medicine) {
        item_result(sid, seq, pb::RESULT_BAD_REQUEST);
        return false;
    }
    if (me->forbid_medicine) {
        item_result(sid, seq, pb::RESULT_WRONG_STATE);
        return false;
    }
    if (me->potion_counter) ++me->potion_count;
    for (const auto& a : item->base) {
        if (a.type == magic_lifepotion_v && a.value[1] > 0) {
            // KNpcAttribModify::LifePotionV: a second potion merges with the one at work
            const int x1 = me->life_state.value, y1 = me->life_state.time, x2 = a.value[0], y2 = a.value[1];
            me->life_state.time = std::max(y1, y2);
            me->life_state.value = (x1 * y1 + x2 * y2) / me->life_state.time;
        }
        // manapotion_v and the rest wait for their attributes (M12)
    }
    if (item->tpl != nullptr && item->tpl->stackable && item->count > 1) {
        --item->count;   // KItemList::SetItemStack(nIdx, nStack - 1)
        item_changed(sid, id);
        return true;
    }
    return take_item(sid, id);
}

bool KSubWorld::item_drop_request(std::uint64_t sid, std::uint32_t id, std::uint32_t seq)
{
    KItemList* list = items_of(sid);
    if (list == nullptr || list->find(id) == nullptr) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    // KPlayer::DropItem (c2s_playerthrowawayitem): the item leaves the bag and lies at the
    // player's feet for anybody; a task item cannot be thrown away
    const KItem* item = list->find(id);
    const KNpc* me = find_player(sid);
    if (me == nullptr || item->genre == KItemGenre::task || !cfg_.objdata) {
        item_result(sid, seq, pb::RESULT_BAD_REQUEST);
        return false;
    }
    KItem copy = *item;
    if (!take_item(sid, id)) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    if (drop_item(std::move(copy), me->pos(), 0).value == 0) {
        log::warn("zone", "dropped item lost", {log::kv("entity", me->id), log::kv("item", id)});
    }
    return true;
}

std::uint32_t KSubWorld::give_item(std::uint64_t sid, KItem item)
{
    KItemList* list = items_of(sid);
    if (list == nullptr) return 0;
    int stacked = 0;
    const std::uint32_t id = list->add_or_stack(std::move(item), room_equipment, &stacked);
    if (id == 0) return 0;
    item_changed(sid, id);
    return id;
}

bool KSubWorld::take_item(std::uint64_t sid, std::uint32_t id)
{
    KItemList* list = items_of(sid);
    if (list == nullptr || !list->remove(id)) return false;
    pb::ItemRemove r;
    r.set_id(id);
    emit({sid}, static_cast<std::uint16_t>(pb::G2C_ITEM_REMOVE), r);
    return true;
}

// The 补血状态 block of KNpc::ProcessState (jx_linux_y 0x0808B7BC): the time counts down every
// frame; every GAME_UPDATE_TIME frames value * percent / 100 life is added, capped at the maximum.
void KSubWorld::process_potions(KNpc& e)
{
    --e.life_state.time;
    if (e.life_state.time % static_cast<int>(kGameUpdateTime) == 0) {
        const std::int64_t before = e.life;
        std::int64_t life = before + static_cast<std::int64_t>(e.life_state.value) * e.life_replenish_percent / 100;
        if (life > e.life_max) life = e.life_max;
        if (life < 0) life = 0;
        e.life = static_cast<std::uint32_t>(life);
        if (life != before) emit_life(e, static_cast<std::int32_t>(life - before), EntityId{});
    }
    if (e.life_state.time <= 0) e.life_state = {};
}

// ---- the ground --------------------------------------------------------------------------

Pos KSubWorld::free_object_pos(Pos at) const
{
    // KSubWorld::GetFreeObjPos: the spot itself, else the nearest cell around it without an
    // object (a ring of 32-unit steps, then a wider one), on walkable ground
    const auto taken = [&](Pos p) {
        bool hit = false;
        grid_.for_each_within(p, 16, [&](EntityId id) {
            const KNpc* o = entities_.find(id);
            if (o != nullptr && o->kind == KNpcKind::drop && std::abs(o->pos().x - p.x) < 16 && std::abs(o->pos().y - p.y) < 16) hit = true;
        });
        return hit;
    };
    const auto ok = [&](Pos p) { return (!cfg_.map || cfg_.map->walkable(p)) && !taken(p); };
    Pos c = clamp(at);
    if (ok(c)) return c;
    for (int ring = 1; ring <= 4; ++ring) {
        for (int dy = -ring; dy <= ring; ++dy) {
            for (int dx = -ring; dx <= ring; ++dx) {
                if (std::abs(dx) != ring && std::abs(dy) != ring) continue;
                const Pos p = clamp(Pos{c.x + dx * 32, c.y + dy * 32});
                if (ok(p)) return p;
            }
        }
    }
    return c;
}

EntityId KSubWorld::drop_item(KItem item, Pos at, std::uint64_t belong)
{
    const KObjTemplate* t = cfg_.objdata && item.tpl ? cfg_.objdata->find(item.tpl->obj) : nullptr;
    if (t == nullptr) {
        log::warn("zone", "no ground object for item", {log::kv("item", item.name()), log::kv("obj", item.tpl ? item.tpl->obj : -1),
                                                        log::kv("objdata", static_cast<bool>(cfg_.objdata))});
        return EntityId{};
    }
    KNpc e;
    e.kind = KNpcKind::drop;
    e.name = item.name();
    e.template_id = static_cast<std::uint32_t>(t->id);
    e.object.kind = KObjKind::item;
    e.object.obj_id = t->id;
    e.object.belong = belong;
    e.object.belong_ticks = belong != 0 ? kObjBelongTime : 0;
    e.object.life_ticks = t->life_time;
    e.object.forever = t->life_time <= 0;
    e.ai_mode = 0;
    e.speed = 0;
    e.life = e.life_max = 1;
    e.home = free_object_pos(at);
    e.set_pos(e.home);
    const Pos home = e.home;
    const EntityId id = entities_.insert(std::move(e));
    entities_.at(id).id = id;
    grid_.insert(id, home);
    ground_items_[id.value] = std::move(item);
    return id;
}

EntityId KSubWorld::drop_money(int amount, Pos at, std::uint64_t belong)
{
    if (amount <= 0 || !cfg_.objdata) return EntityId{};
    const KObjTemplate* t = cfg_.objdata->find(cfg_.objdata->money_obj(amount));
    if (t == nullptr) return EntityId{};
    KNpc e;
    e.kind = KNpcKind::drop;
    e.name = std::to_string(amount) + " lượng";
    e.template_id = static_cast<std::uint32_t>(t->id);
    e.object.kind = KObjKind::money;
    e.object.obj_id = t->id;
    e.object.money = amount;
    e.object.belong = belong;
    e.object.belong_ticks = belong != 0 ? kObjBelongTime : 0;
    e.object.life_ticks = t->life_time;
    e.object.forever = t->life_time <= 0;
    e.speed = 0;
    e.life = e.life_max = 1;
    e.home = free_object_pos(at);
    e.set_pos(e.home);
    const Pos home = e.home;
    const EntityId id = entities_.insert(std::move(e));
    entities_.at(id).id = id;
    grid_.insert(id, home);
    ++ground_money_;
    return id;
}

const KItem* KSubWorld::ground_item(EntityId object) const
{
    const auto it = ground_items_.find(object.value);
    return it == ground_items_.end() ? nullptr : &it->second;
}

void KSubWorld::object_tick(KNpc& e, std::vector<EntityId>& expired)
{
    if (e.object.belong != 0 && e.object.belong_ticks > 0 && --e.object.belong_ticks <= 0) {
        e.object.belong = 0;   // anybody may take it now
    }
    if (!e.object.forever && --e.object.life_ticks <= 0) expired.push_back(e.id);
}

void KSubWorld::remove_object(EntityId id)
{
    KNpc* e = entities_.find(id);
    if (e == nullptr || e->kind != KNpcKind::drop) return;
    entity_gone(*e);
    grid_.remove(id);
    if (e->object.kind == KObjKind::money && ground_money_ > 0) --ground_money_;
    ground_items_.erase(id.value);
    entities_.destroy(id);
}

bool KSubWorld::pick_up_request(std::uint64_t sid, EntityId object, std::uint32_t seq)
{
    const auto pit = players_.find(sid);
    KNpc* me = pit == players_.end() ? nullptr : find_mutable(pit->second);
    KNpc* o = find_mutable(object);
    if (me == nullptr || o == nullptr || o->kind != KNpcKind::drop) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    // kept for somebody else (the team share of the old server is not here yet)
    if (o->object.belong != 0 && o->object.belong != me->player_id) {
        item_result(sid, seq, pb::RESULT_UNAUTHORIZED);
        return false;
    }
    const std::int64_t dx = me->pos().x - o->pos().x;
    const std::int64_t dy = me->pos().y - o->pos().y;
    if (dx * dx + dy * dy > kPickUpDistance2) {
        item_result(sid, seq, pb::RESULT_WRONG_STATE);   // enumMSG_ID_OBJ_TOO_FAR
        return false;
    }
    if (o->object.kind == KObjKind::money) {
        KItemList* list = items_of(sid);
        if (list == nullptr || !list->add_money(room_equipment, o->object.money)) {   // KPlayer::Earn
            item_result(sid, seq, pb::RESULT_FULL);
            return false;
        }
        send_money(sid);
        log::debug("zone", "money picked up", {log::kv("entity", me->id), log::kv("money", o->object.money)});
        remove_object(object);
        return true;
    }
    const auto it = ground_items_.find(object.value);
    if (it == ground_items_.end()) {
        item_result(sid, seq, pb::RESULT_NOT_FOUND);
        return false;
    }
    KItem item = it->second;
    item.id = 0;   // a new id in this player's list
    const std::uint32_t id = give_item(sid, std::move(item));
    if (id == 0) {
        item_result(sid, seq, pb::RESULT_FULL);   // no room in the bag: it stays on the ground
        return false;
    }
    log::debug("zone", "item picked up", {log::kv("entity", me->id), log::kv("item", id)});
    remove_object(object);
    return true;
}

// KNpc::OnDeath of the JX2 server (jx_linux_y 0x08088B60): a monster a player killed rolls
// m_CurrentTreasure times - g_Random(100) < MoneyRate is a pile of money (KNpc::LoseMoney:
// experience * MoneyScale / 100, then x the server's MoneyRate / 100), else one item of the table
// (KNpc::LoseSingleItem).  Everything lies for the killer first (SetItemBelong).  The team share
// ([Main] IsTeamShare) waits for the team system.
void KSubWorld::lose_treasure(KNpc& dead, EntityId killer)
{
    if (dead.kind == KNpcKind::player || dead.treasure <= 0 || !cfg_.templates) return;
    const KNpc* k = entities_.find(killer);
    if (k == nullptr || k->kind != KNpcKind::player) return;
    const KNpcTemplate* t = cfg_.templates->find(dead.template_id);
    const KNpcDropRate* table = t && !t->drop_rate_file.empty() ? cfg_.templates->drop_rate(t->drop_rate_file) : nullptr;
    if (table == nullptr) return;
    int items = 0, money = 0;
    for (int i = 0; i < dead.treasure; ++i) {
        if (random_percent() < static_cast<std::uint32_t>(table->money_rate)) {
            const std::int64_t amount = static_cast<std::int64_t>(dead.exp) * table->money_scale / 100 * cfg_.money_rate_percent / 100;
            if (amount > 0) {
                pending_drops_.push_back(KPendingDrop{std::nullopt, static_cast<int>(amount), dead.pos(), k->player_id});
                ++money;
            }
        } else if (auto item = gen_random_item(*table, static_cast<int>(dead.level), static_cast<int>(dead.series), 0)) {
            pending_drops_.push_back(KPendingDrop{std::move(item), 0, dead.pos(), k->player_id});
            ++items;
        }
    }
    log::debug("zone.fight", "treasure dropped", {log::kv("entity", dead.id), log::kv("killer", killer), log::kv("rolls", dead.treasure),
                                                   log::kv("items", items), log::kv("money", money)});
}

void KSubWorld::flush_pending_drops()
{
    if (pending_drops_.empty()) return;
    std::vector<KPendingDrop> drops;
    drops.swap(pending_drops_);
    for (auto& d : drops) {
        if (d.item) drop_item(std::move(*d.item), d.at, d.belong);
        else drop_money(d.money, d.at, d.belong);
    }
}

// GenRandomItem (jx_linux_y 0x08083BB0): a weighted entry, its level from the npc level through
// the table's scales (clamped to the table's range, then 1..10), then the prefix / suffix levels
// of the piece: k = g_Random(4) + 3 and every slot i <= k gets the item level (4, 5 or 6 magic
// slots - the tail of six beyond k stays 0); an entry with its own MagicLevel1..6 uses those
// instead.  An equipment entry of quality 0 whose EnchasableRate roll hits, or of quality 2,
// gets MinSocket..MaxSocket sockets (-1 slots) and becomes quality 2 - platina, which is not
// made yet (the tables of the JX2 server set EnchasableRate 0 everywhere).  Quality 1 is a gold
// row of goldequip.txt (Gen_GoldEquip; the entry's Detail is its 1-based row).  KItemSet::Add
// then makes the item from the current table version with the killer's luck.
std::optional<KItem> KSubWorld::gen_random_item(const KNpcDropRate& table, int npc_level, int npc_series, int luck)
{
    if (table.max_level_scale <= 0 || table.min_level_scale <= 0 || table.rand_range <= 0 || table.entries.empty()) return std::nullopt;
    const auto roll = static_cast<int>(rng_() % static_cast<std::uint32_t>(table.rand_range));
    const KDropEntry* pick = nullptr;
    int sum = 0;
    for (const auto& e : table.entries) {
        if (roll >= sum && roll < sum + e.rate) {
            pick = &e;
            break;
        }
        sum += e.rate;
    }
    if (pick == nullptr) return std::nullopt;   // the range beyond the entries: nothing
    const int series = pick->series >= 0 ? pick->series : table.series >= 0 ? table.series : npc_series;
    int lo, hi;
    if (pick->min_level >= 0) {
        lo = pick->min_level;
        hi = pick->max_level >= lo ? pick->max_level : lo;
    } else {
        lo = (npc_level - 1) / table.max_level_scale + 1;
        hi = (npc_level - 1) / table.min_level_scale + 1;
        if (lo > hi) std::swap(lo, hi);
        lo = std::clamp(lo, table.min_level, table.max_level);
        hi = std::clamp(hi, table.min_level, table.max_level);
        if (lo > hi) std::swap(lo, hi);
    }
    int level = lo + static_cast<int>(rng_() % static_cast<std::uint32_t>(hi - lo + 1));
    level = std::clamp(level, 1, 10);
    // the magic slots (pnaryMALevel)
    int quality = pick->quality;
    const int enchasable_rate = pick->enchasable_rate >= 0 ? pick->enchasable_rate : table.enchasable_rate;
    const int min_socket = pick->min_socket >= 0 ? pick->min_socket : table.min_socket;
    const int max_socket = pick->max_socket >= 0 ? pick->max_socket : table.max_socket;
    bool sockets = false;
    if (pick->genre == static_cast<int>(KItemGenre::equip)) {
        if (quality == 0) sockets = static_cast<int>(random_percent()) < enchasable_rate;
        else if (quality == 2) sockets = true;
    }
    KMagicLevels levels{};
    if (!sockets) {
        const int k = random(4) + 3;
        for (int i = 0; i < 6; ++i) levels[static_cast<std::size_t>(i)] = i <= k ? level : 0;
    } else {
        const int n = min_socket + random(max_socket + 1 - min_socket);
        for (int i = 0; i < n && i < 6; ++i) levels[static_cast<std::size_t>(i)] = -1;
        quality = 2;
    }
    if (std::any_of(pick->magic_level.begin(), pick->magic_level.end(), [](int l) { return l > 0; })) levels = pick->magic_level;
    auto gen = item_generator(item_version());
    if (!gen) return std::nullopt;
    std::optional<KItem> item;
    switch (static_cast<KItemGenre>(pick->genre)) {
    case KItemGenre::equip:
        if (quality == 0) item = gen->equipment(pick->detail, pick->particular, series, level, &levels, luck);
        else if (quality == 1) item = gen->gold(luck, pick->detail);
        else log::debug("zone.fight", "drop quality not made yet", {log::kv("table", table.source), log::kv("quality", quality)});
        break;
    case KItemGenre::medicine: item = gen->medicine(pick->detail, level); break;
    case KItemGenre::task: item = gen->quest(pick->detail, 1); break;
    case KItemGenre::town_portal: item = gen->town_portal(); break;
    case KItemGenre::magic_script: item = gen->magic_script(pick->detail, pick->particular, level, series, 1); break;
    default: break;
    }
    if (!item) {
        log::debug("zone.fight", "drop row missing", {log::kv("table", table.source), log::kv("genre", pick->genre), log::kv("detail", pick->detail),
                                                       log::kv("particular", pick->particular), log::kv("level", level), log::kv("quality", quality)});
        return std::nullopt;
    }
    return item;
}

void KSubWorld::save_items(std::uint64_t sid, pb::RoleData& out) const
{
    const auto it = items_.find(sid);
    if (it == items_.end()) return;
    const KItemList& list = it->second;
    out.clear_items();
    list.each([&](const KItem& item, const KItemPlace& place) {
        pb::ItemData* data = out.add_items();
        item.to_proto(*data);
        data->set_room(static_cast<std::uint32_t>(place.room));
        data->set_x(static_cast<std::uint32_t>(place.x));
        data->set_y(static_cast<std::uint32_t>(place.y));
    });
    out.set_next_item_id(list.next_id());
    out.set_money(static_cast<std::uint32_t>(list.money(room_equipment)));
    out.set_bank_money(static_cast<std::uint32_t>(list.money(room_repository)));
}

} // namespace jx::zone
