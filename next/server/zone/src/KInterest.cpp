// Interest management of a map: who is told about what.
//
// The part of KSubWorld that decides which entities each client knows.  It replaces the broadcast
// rule of the old server - KRegion::BroadCast, at most MAX_BROADCAST_COUNT = 100 receivers per
// packet (Core/Src/KRegion.h:9) - and the first port of it (N1), which cut every packet to the 100
// nearest sessions.  Both bound the traffic of a crowd, and both forget who was told what:
//
//   - a newcomer was sent EVERYTHING in view, but its own arrival only reached 100 sessions;
//   - a departure, a death, a corpse disappearing only reached 100 sessions, so everybody else
//     kept a ghost, or a monster that stood alive for ever;
//   - the 100 changed from packet to packet, so clients received moves for entities they had
//     never been sent.
//
// The old client survived that because it asked for what it did not know and dropped what went
// silent.  Here the server simply never creates the situation: the limit sits on the client's
// side.  Each client KNOWS at most max_viewers other players (plus max_known_npcs of everything
// else), nearest first; updates go to exactly the clients that know the entity (KNpc::watchers),
// and so does its despawn.  The traffic is bounded the same way - the sum of all watcher lists
// is at most players x limit - and nothing can go stale.
//
// Looking around costs one pass over the entities in view, so it is not done for everybody every
// tick: a client looks when it arrives, when it changes cell, while it is still catching up with
// a packed place (spawn_budget entities per look, which is what spreads a crowd's arrival over
// several ticks - N4), and otherwise every interest_period ticks.
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

#include "jx/msg.pb.h"

namespace jx::zone {

namespace {

bool by_id(EntityId a, EntityId b) noexcept { return a.value < b.value; }

bool knows(const KViewer& v, EntityId id)
{
    return std::binary_search(v.known.begin(), v.known.end(), id, by_id);
}

void insert_sorted(std::vector<EntityId>& list, EntityId id)
{
    list.insert(std::lower_bound(list.begin(), list.end(), id, by_id), id);
}

void insert_sorted(std::vector<std::uint64_t>& list, std::uint64_t sid)
{
    const auto it = std::lower_bound(list.begin(), list.end(), sid);
    if (it == list.end() || *it != sid) list.insert(it, sid);
}

void erase_sorted(std::vector<std::uint64_t>& list, std::uint64_t sid)
{
    const auto it = std::lower_bound(list.begin(), list.end(), sid);
    if (it != list.end() && *it == sid) list.erase(it);
}

std::int64_t dist2(Pos a, Pos b) noexcept
{
    const std::int64_t dx = static_cast<std::int64_t>(a.x) - b.x;
    const std::int64_t dy = static_cast<std::int64_t>(a.y) - b.y;
    return dx * dx + dy * dy;
}

} // namespace

const KViewer* KSubWorld::viewer(std::uint64_t sid) const
{
    const auto it = viewers_.find(sid);
    return it == viewers_.end() ? nullptr : &it->second;
}

void KSubWorld::run_interest()
{
    scratch_due_.clear();
    for (const auto& [sid, v] : viewers_) {
        if (v.dirty || tick_ >= v.next_look) {
            scratch_due_.push_back(sid);
            if (v.dirty) ++look_stats_.looks_dirty;
        }
    }
    // the map is unordered; the packets a recording replays must not depend on its layout
    std::sort(scratch_due_.begin(), scratch_due_.end());
    for (const std::uint64_t sid : scratch_due_) {
        const auto it = viewers_.find(sid);
        if (it != viewers_.end()) look_around(sid, it->second);
    }
}

void KSubWorld::look_around(std::uint64_t sid, KViewer& v)
{
    v.dirty = false;
    v.next_look = tick_ + std::max<std::uint32_t>(1, cfg_.interest_period);
    KNpc* me = entities_.find(v.self);
    if (me == nullptr) return;
    ++look_stats_.looks;
    look_stats_.known_checked += v.known.size();
    const Pos at = me->pos();
    const Cell here = grid_.cell_of(at);

    const std::int32_t keep_x = grid_.view_cells_x() + std::max(0, cfg_.view_slack);
    const std::int32_t keep_y = grid_.view_cells_y() + std::max(0, cfg_.view_slack);

    pb::EntityDespawn vanish;
    pb::EntitySpawn appear;
    const auto forget = [&](EntityId id, KNpc* e) {
        ++look_stats_.forgotten;
        vanish.add_entity_ids(id.value);
        if (e != nullptr) erase_sorted(e->watchers, sid);
    };
    const auto learn = [&](KNpc& e) {
        ++look_stats_.learned;
        insert_sorted(v.known, e.id);
        insert_sorted(e.watchers, sid);
        if (e.kind == KNpcKind::player && e.id != v.self) ++v.known_players;
        fill_info(e, *appear.add_entities());
    };

    // 1. What left.  An entity is only given up view_slack cells OUTSIDE the view, so somebody
    //    pacing along a cell edge does not appear and vanish with every step.
    std::size_t kept = 0;
    std::int32_t players_kept = 0;
    for (const EntityId id : v.known) {
        if (id == v.self) {
            v.known[kept++] = id;
            continue;
        }
        // Only the distance is asked here.  An entity that left the WORLD - logged out, a corpse
        // taken away - was removed from every client by entity_gone() the moment it happened, so
        // this loop never meets one; the cell comes from the position, not from a lookup.
        KNpc* e = entities_.find(id);
        const Cell c = e != nullptr ? grid_.cell_of(e->pos()) : Cell{};
        if (e == nullptr || std::abs(c.cx - here.cx) > keep_x || std::abs(c.cy - here.cy) > keep_y || invisible_to(*e, v.self)) {
            forget(id, e);   // (hidden - [hide] 200 - it is seen by its own client only, 0x08079200)
        } else {
            v.known[kept++] = id;
            if (e->kind == KNpcKind::player) ++players_kept;
        }
    }
    const bool forgot = kept < v.known.size();
    v.known.resize(kept);
    v.known_players = players_kept;   // counted, not carried: a count that drifts would silently shrink the limit

    const auto nearer = [](const Near& a, const Near& b) { return a.dist2 != b.dist2 ? a.dist2 < b.dist2 : a.id.value < b.id.value; };

    // 2. How much room there is.  0 = no limit.
    const std::int32_t unlimited = std::numeric_limits<std::int32_t>::max();
    const std::int32_t known_npcs = static_cast<std::int32_t>(v.known.size()) - v.known_players - (knows(v, v.self) ? 1 : 0);
    std::int32_t room_players = cfg_.max_viewers > 0 ? std::max(0, cfg_.max_viewers - v.known_players) : unlimited;
    const std::int32_t room_npcs = cfg_.max_known_npcs > 0 ? std::max(0, cfg_.max_known_npcs - known_npcs) : unlimited;
    std::int32_t budget = std::max(1, cfg_.spawn_budget);

    // The client's own character comes first, always, and does not count against anything.
    if (!knows(v, v.self)) {
        learn(*me);
        --budget;
    }

    // 3. What is near and not known yet.  The grid files players apart from the rest, and each
    //    kind is only looked at when it can be used: measured with 3000 players on one spot, a look
    //    that walked over all of them cost 80 microseconds, and 750 clients look every tick.
    //    And only the cells that changed since the last look are walked: a cell whose version is
    //    the same holds exactly the entities it held then, each of which was learned or rejected -
    //    unless something was forgotten or the room grew meanwhile, then everything is looked at
    //    again.  Measured with 1500 players and 1500 wandering npcs in one town: every look walked
    //    ~300 npcs to learn nothing, a third of the interest pass.
    scratch_near_players_.clear();
    scratch_near_npcs_.clear();
    const auto candidate = [&](std::vector<Near>& into, EntityId id) {
        ++look_stats_.candidates;
        if (id == v.self || knows(v, id)) return;
        const KNpc* e = entities_.find(id);
        if (e == nullptr || invisible_to(*e, v.self)) return;   // hidden ([hide] 200): seen by its own client only
        into.push_back(Near{dist2(at, e->pos()), id});
    };
    const std::size_t cells = grid_.view_cell_count();
    const bool everything = !v.looked || v.carry || forgot || here != v.last_cell || v.cell_versions.size() != cells ||
                            room_players > v.last_room_players || room_npcs > v.last_room_npcs;
    if (v.cell_versions.size() != cells) v.cell_versions.assign(cells, ~std::uint64_t{0});
    const bool want_npcs_scan = room_npcs > 0;
    const bool want_players_scan = room_players > 0;
    std::size_t cells_walked = 0;
    grid_.for_each_view_cell(here, [&](std::size_t index, const KRegionGrid::CellView& cell) {
        const std::uint64_t version = (static_cast<std::uint64_t>(cell.players_version) << 32) | cell.others_version;
        const bool changed = everything || version != v.cell_versions[index];
        v.cell_versions[index] = version;
        if (!changed) return;
        ++cells_walked;
        if (want_npcs_scan) for (const EntityId id : *cell.others) candidate(scratch_near_npcs_, id);
        if (want_players_scan) for (const EntityId id : *cell.players) candidate(scratch_near_players_, id);
    });
    if (cells_walked == 0) ++look_stats_.looks_idle;
    v.looked = true;
    v.carry = false;
    v.last_cell = here;
    v.last_room_players = room_players;
    v.last_room_npcs = room_npcs;
    if (room_players > 0) {
        if (static_cast<std::int64_t>(scratch_near_players_.size()) > room_players) ++viewers_capped_;
    } else if (cfg_.max_viewers > 0 && tick_ >= v.next_swap) {
        v.next_swap = tick_ + static_cast<std::uint64_t>(std::max<std::uint32_t>(1, cfg_.interest_period)) * kSwapEveryLooks;
        // A full client still has to notice who walks right up to it: a player that came within
        // near_radius takes the place of a known one that is outside it - and only that.  The
        // first rule here traded a known player for any unknown one less than half as far away;
        // in a dense crowd such a trade is always available, so every client swapped four players
        // every 16 ticks for ever: 700 despawn+spawn pairs a tick at 3000 players on one spot,
        // most of the interest pass and twice the traffic, for players the screen could not tell
        // apart.  With the near radius the trades stop the moment everybody known is near, or
        // everybody near is known.
        ++viewers_capped_;
        const std::int64_t near2 = near_radius2();
        scratch_far_known_.clear();
        for (const EntityId id : v.known) {
            if (id == v.self) continue;
            const KNpc* e = entities_.find(id);
            if (e == nullptr || e->kind != KNpcKind::player) continue;
            const std::int64_t d2 = dist2(at, e->pos());
            if (d2 > near2) scratch_far_known_.push_back(Near{d2, id});
        }
        const std::size_t far_n = std::min<std::size_t>(kSwapsPerLook, scratch_far_known_.size());
        if (far_n > 0) {
            std::partial_sort(scratch_far_known_.begin(), scratch_far_known_.begin() + static_cast<std::ptrdiff_t>(far_n),
                              scratch_far_known_.end(), [&](const Near& a, const Near& b) { return nearer(b, a); });
            // any unknown player within the radius will do; the first few of the nearest cells
            // are as good as the nearest of all, and cost nothing next to walking a whole crowd
            grid_.for_each_player_near(at, near_radius(), [&](EntityId id) {
                if (id == v.self || knows(v, id)) return true;
                const KNpc* e = entities_.find(id);
                if (e == nullptr) return true;
                const std::int64_t d2 = dist2(at, e->pos());
                if (d2 <= near2) scratch_near_players_.push_back(Near{d2, id});
                return scratch_near_players_.size() < far_n * 2;
            });
            const std::size_t top = std::min<std::size_t>(far_n, scratch_near_players_.size());
            std::partial_sort(scratch_near_players_.begin(), scratch_near_players_.begin() + static_cast<std::ptrdiff_t>(top),
                              scratch_near_players_.end(), nearer);
            for (std::size_t i = 0; i < top && budget > 0; ++i) {
                const Near& in = scratch_near_players_[i];
                const Near& out = scratch_far_known_[i];
                forget(out.id, entities_.find(out.id));
                v.known.erase(std::lower_bound(v.known.begin(), v.known.end(), out.id, by_id));
                --v.known_players;
                learn(*entities_.find(in.id));
                --budget;
            }
        }
        scratch_near_players_.clear();   // the swaps are done: nothing else can be added to a full client
    }

    // 4. Learn the nearest first, players and the rest together, as far as room and budget go.
    const auto take = [&](std::vector<Near>& list, std::int32_t room) {
        const std::size_t n = std::min<std::size_t>(list.size(), static_cast<std::size_t>(std::min(room, budget)));
        std::partial_sort(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(n), list.end(), nearer);
        return n;
    };
    const std::size_t want_players = std::min<std::size_t>(scratch_near_players_.size(), static_cast<std::size_t>(room_players));
    const std::size_t want_npcs = std::min<std::size_t>(scratch_near_npcs_.size(), static_cast<std::size_t>(room_npcs));
    const std::size_t sorted_players = take(scratch_near_players_, room_players);
    const std::size_t sorted_npcs = take(scratch_near_npcs_, room_npcs);
    std::size_t ip = 0, in = 0;
    while (budget > 0 && (ip < sorted_players || in < sorted_npcs)) {
        const bool player_next = in >= sorted_npcs || (ip < sorted_players && nearer(scratch_near_players_[ip], scratch_near_npcs_[in]));
        const EntityId id = player_next ? scratch_near_players_[ip++].id : scratch_near_npcs_[in++].id;
        if (KNpc* e = entities_.find(id)) {
            learn(*e);
            --budget;
        }
    }
    // More fits than the budget allowed: carry on next tick instead of waiting for the routine look.
    if (ip < want_players || in < want_npcs) {
        v.dirty = true;
        v.carry = true;
        ++look_stats_.looks_carry;
    }

    if (vanish.entity_ids_size() > 0) emit({sid}, static_cast<std::uint16_t>(pb::G2C_ENTITY_DESPAWN), vanish);
    if (appear.entities_size() > 0) emit_spawn({sid}, appear);
}

std::int32_t KSubWorld::near_radius() const noexcept
{
    return cfg_.near_radius > 0 ? cfg_.near_radius : std::max(1, cfg_.view_width / 4);
}

std::int64_t KSubWorld::near_radius2() const noexcept
{
    const std::int64_t r = near_radius();
    return r * r;
}

// The movements that were held back because they happened far from the client (emit_move, N3):
// every far_period ticks each client gets them in one frame, the latest state of each mover.
void KSubWorld::flush_far()
{
    if (cfg_.far_period <= 1) return;
    scratch_due_.clear();
    for (const auto& [sid, v] : viewers_) {
        if (!v.far_pending.empty() && tick_ >= v.next_far_flush) scratch_due_.push_back(sid);
    }
    std::sort(scratch_due_.begin(), scratch_due_.end());   // packets in an order a recording can replay
    pb::EntityMoves batch;
    for (const std::uint64_t sid : scratch_due_) {
        KViewer& v = viewers_.find(sid)->second;
        batch.clear_moves();
        for (const EntityId id : v.far_pending) {
            // dropped from view since it moved: the client was sent its despawn, nothing to say
            if (!knows(v, id)) continue;
            if (const KNpc* e = entities_.find(id)) fill_move(*e, *batch.add_moves());
        }
        v.far_pending.clear();
        v.next_far_flush = tick_ + cfg_.far_period;
        if (batch.moves_size() > 0) emit({sid}, static_cast<std::uint16_t>(pb::G2C_ENTITY_MOVES), batch);
    }
}

// The entity leaves the world - a player logs out or changes map, a corpse is taken away until it
// revives, a jump carries it somewhere else.  Every client that knows it is told, and forgets it.
// keep_self: the player's own client keeps its character (a jump within the map).
void KSubWorld::entity_gone(KNpc& e, bool keep_self)
{
    if (e.watchers.empty()) return;
    std::vector<std::uint64_t> told;
    told.reserve(e.watchers.size());
    for (const std::uint64_t sid : e.watchers) {
        if (keep_self && sid == e.sid) continue;
        const auto it = viewers_.find(sid);
        if (it == viewers_.end()) continue;
        KViewer& v = it->second;
        const auto pos = std::lower_bound(v.known.begin(), v.known.end(), e.id, by_id);
        if (pos == v.known.end() || *pos != e.id) continue;
        v.known.erase(pos);
        if (e.kind == KNpcKind::player && e.id != v.self) --v.known_players;
        if (sid != e.sid) told.push_back(sid);   // a client is never sent the despawn of its own character
    }
    // with keep_self the loop above skipped the player's own session, so its `known` still holds it
    const bool self_stays = keep_self && e.sid != 0 && std::binary_search(e.watchers.begin(), e.watchers.end(), e.sid);
    e.watchers.clear();
    if (self_stays) e.watchers.push_back(e.sid);
    if (told.empty()) return;
    pb::EntityDespawn gone;
    gone.add_entity_ids(e.id.value);
    emit(std::move(told), static_cast<std::uint16_t>(pb::G2C_ENTITY_DESPAWN), gone);
}

// The viewers whose view holds this npc look again at the next tick, at everything (its cell's
// version did not change, so a plain look would walk past it): KNpc::SetHide 0x0807FF80 when the
// hiding ends re-syncs the npc to the players around.
void KSubWorld::wake_viewers_near(const KNpc& e)
{
    const Cell c = grid_.cell_of(e.pos());
    const std::int32_t vx = grid_.view_cells_x() + std::max(0, cfg_.view_slack);
    const std::int32_t vy = grid_.view_cells_y() + std::max(0, cfg_.view_slack);
    for (auto& kv : viewers_) {
        KViewer& v = kv.second;
        const KNpc* me = entities_.find(v.self);
        if (me == nullptr) continue;
        const Cell here = grid_.cell_of(me->pos());
        if (std::abs(c.cx - here.cx) > vx || std::abs(c.cy - here.cy) > vy) continue;
        v.dirty = true;
        v.carry = true;
    }
}

// The session leaves this map: it watches nothing any more.
void KSubWorld::drop_viewer(std::uint64_t sid)
{
    const auto it = viewers_.find(sid);
    if (it == viewers_.end()) return;
    for (const EntityId id : it->second.known) {
        if (KNpc* e = entities_.find(id)) erase_sorted(e->watchers, sid);
    }
    viewers_.erase(it);
}

} // namespace jx::zone
