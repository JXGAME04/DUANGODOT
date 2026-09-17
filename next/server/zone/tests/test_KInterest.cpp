// What a client is told must stay consistent with what it was told before.
//
// These tests do not look at the server's data structures.  They replay the packets each session
// receives, exactly as a client would, and check the two promises a client relies on:
//
//   1. every update (move, action, life) is about an entity it has been sent a spawn for;
//   2. every entity it was sent a spawn for is taken away again when it leaves - no ghosts.
//
// The first version of the viewer cap (N1) broke both: a newcomer was sent everything in view, but
// its own arrival, its departure and every death only reached the 100 nearest sessions.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KSubWorld.h"

using jx::EntityId;
using jx::zone::KSubWorld;
using jx::zone::KSubWorldConfig;
using jx::zone::Packet;
using jx::zone::Pos;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::warn;
        jx::log::init(o);
    }
};

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    return r;
}

// Every connected client, as the packets describe it.
struct Clients {
    std::map<std::uint64_t, std::set<std::uint64_t>> known;   // sid -> entity ids it has a spawn for
    std::vector<std::string> violations;
    std::uint64_t deliveries = 0;

    void connect(std::uint64_t sid) { known[sid]; }
    void disconnect(std::uint64_t sid) { known.erase(sid); }

    void complain(std::uint64_t sid, const char* what, std::uint64_t entity)
    {
        if (violations.size() < 10) {
            violations.push_back("sid " + std::to_string(sid) + ": " + what + " for entity " + std::to_string(entity));
        }
    }

    void apply(const std::vector<Packet>& out)
    {
        for (const Packet& p : out) {
            deliveries += p.sids.size();
            for (const std::uint64_t sid : p.sids) {
                const auto it = known.find(sid);
                if (it == known.end()) continue;   // already gone: the gateway drops it
                std::set<std::uint64_t>& mine = it->second;
                switch (p.msg_id) {
                case jx::pb::G2C_ENTITY_SPAWN: {
                    jx::pb::EntitySpawn m;
                    REQUIRE(m.ParseFromString(p.payload));
                    for (const auto& e : m.entities()) {
                        if (!mine.insert(e.entity_id()).second) complain(sid, "second spawn", e.entity_id());
                    }
                    break;
                }
                case jx::pb::G2C_ENTITY_DESPAWN: {
                    jx::pb::EntityDespawn m;
                    REQUIRE(m.ParseFromString(p.payload));
                    for (const std::uint64_t id : m.entity_ids()) {
                        if (mine.erase(id) == 0) complain(sid, "despawn of something never sent", id);
                    }
                    break;
                }
                case jx::pb::G2C_ENTITY_MOVE: {
                    jx::pb::EntityMove m;
                    REQUIRE(m.ParseFromString(p.payload));
                    if (!mine.contains(m.entity_id())) complain(sid, "move", m.entity_id());
                    break;
                }
                case jx::pb::G2C_ENTITY_ACTION: {
                    jx::pb::EntityAction m;
                    REQUIRE(m.ParseFromString(p.payload));
                    if (!mine.contains(m.entity_id())) complain(sid, "action", m.entity_id());
                    break;
                }
                case jx::pb::G2C_ENTITY_LIFE: {
                    jx::pb::EntityLife m;
                    REQUIRE(m.ParseFromString(p.payload));
                    if (!mine.contains(m.entity_id())) complain(sid, "life", m.entity_id());
                    break;
                }
                default: break;
                }
            }
        }
    }

    [[nodiscard]] std::size_t who_knows(std::uint64_t entity) const
    {
        std::size_t n = 0;
        for (const auto& [sid, set] : known) n += set.contains(entity) ? 1u : 0u;
        return n;
    }
};

KSubWorldConfig crowd_world(std::int32_t cap)
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 20000;
    c.height = 20000;
    c.spawn_point = Pos{10000, 10000};
    c.max_viewers = cap;
    return c;
}

} // namespace

TEST_CASE("a player that leaves a crowd leaves no ghost behind", "[world][aoi][interest]")
{
    Quiet q;
    const KSubWorldConfig c = crowd_world(8);
    KSubWorld w(c);
    Clients clients;
    std::map<std::uint64_t, EntityId> entity_of;

    for (std::uint64_t sid = 1; sid <= 40; ++sid) {
        EntityId id;
        Pos p;
        const Pos at{c.spawn_point.x + static_cast<std::int32_t>(sid) * 4, c.spawn_point.y};
        clients.connect(sid);
        REQUIRE(w.spawn_player(sid, role(100 + sid, "P" + std::to_string(sid), at), id, p) == jx::pb::RESULT_OK);
        entity_of[sid] = id;
        clients.apply(w.take_outbox());
        w.tick();
        clients.apply(w.take_outbox());
    }
    for (int i = 0; i < 20; ++i) {   // let every client finish looking around
        w.tick();
        clients.apply(w.take_outbox());
    }
    REQUIRE(clients.violations.empty());

    // every client knows itself, and nobody knows more players than the cap allows (+ itself)
    for (const auto& [sid, set] : clients.known) {
        INFO("sid " << sid);
        CHECK(set.contains(entity_of[sid].value));
        CHECK(set.size() <= static_cast<std::size_t>(c.max_viewers) + 1);
    }

    const std::uint64_t leaver = entity_of[20].value;
    REQUIRE(clients.who_knows(leaver) > 1);
    clients.disconnect(20);
    REQUIRE(w.remove_player(20));
    clients.apply(w.take_outbox());
    for (int i = 0; i < 10; ++i) {
        w.tick();
        clients.apply(w.take_outbox());
    }
    CHECK(clients.who_knows(leaver) == 0);   // the ghost test
    for (const std::string& v : clients.violations) FAIL_CHECK(v);
}

TEST_CASE("a crowd moving, fighting, joining and leaving never breaks what a client knows", "[world][aoi][interest]")
{
    Quiet q;
    KSubWorldConfig c = crowd_world(6);
    c.max_known_npcs = 5;
    KSubWorld w(c);
    Clients clients;
    std::map<std::uint64_t, EntityId> entity_of;
    std::minstd_rand rng(12345);
    const auto rnd = [&](int n) { return static_cast<std::int32_t>(rng() % static_cast<std::uint32_t>(n)); };

    // a few monsters in the middle of the crowd: they wander, get hit, die and come back
    std::vector<EntityId> monsters;
    for (int i = 0; i < 12; ++i) {
        monsters.push_back(w.spawn_npc("M" + std::to_string(i), Pos{c.spawn_point.x + rnd(600) - 300, c.spawn_point.y + rnd(600) - 300},
                                       0, 200, jx::zone::KNpcKind::monster));
    }

    std::uint64_t next_sid = 1;
    const auto join = [&]() {
        const std::uint64_t sid = next_sid++;
        EntityId id;
        Pos p;
        clients.connect(sid);
        const Pos at{c.spawn_point.x + rnd(2400) - 1200, c.spawn_point.y + rnd(2400) - 1200};
        REQUIRE(w.spawn_player(sid, role(100 + sid, "P" + std::to_string(sid), at), id, p) == jx::pb::RESULT_OK);
        entity_of[sid] = id;
        clients.apply(w.take_outbox());
    };
    for (int i = 0; i < 50; ++i) join();

    for (int t = 0; t < 400; ++t) {
        // a handful of commands per tick, like a real crowd
        for (int k = 0; k < 6; ++k) {
            if (entity_of.empty()) break;
            auto it = entity_of.begin();
            std::advance(it, rnd(static_cast<int>(entity_of.size())));
            const std::uint64_t sid = it->first;
            switch (rnd(10)) {
            case 0:   // leave
                clients.disconnect(sid);
                REQUIRE(w.remove_player(sid));
                entity_of.erase(it);
                break;
            case 1:
                join();
                break;
            case 2:
            case 3:   // hit a monster
                w.attack_request(sid, monsters[static_cast<std::size_t>(rnd(static_cast<int>(monsters.size())))], static_cast<std::uint32_t>(t));
                break;
            default:  // walk, sometimes far enough to change who is in view
                w.move_request(sid, Pos{c.spawn_point.x + rnd(3000) - 1500, c.spawn_point.y + rnd(3000) - 1500}, static_cast<std::uint32_t>(t));
                break;
            }
            clients.apply(w.take_outbox());
        }
        w.tick();
        clients.apply(w.take_outbox());
    }
    for (const std::string& v : clients.violations) FAIL_CHECK(v);

    // and what each client holds at the end is real: it exists
    for (const auto& [sid, set] : clients.known) {
        for (const std::uint64_t id : set) {
            INFO("sid " << sid << " entity " << id);
            CHECK(w.find_entity(EntityId{id}) != nullptr);
        }
    }
}

TEST_CASE("traffic grows with the crowd, not with its square", "[world][aoi][interest]")
{
    // MASTER SPEC 69 and the old MAX_BROADCAST_COUNT: with everybody on one spot, one line of chat
    // from each player must cost about players x cap deliveries, not players x players.
    Quiet q;
    const KSubWorldConfig c = crowd_world(8);
    KSubWorld w(c);
    const std::uint64_t players = 120;
    for (std::uint64_t sid = 1; sid <= players; ++sid) {
        EntityId id;
        Pos p;
        const Pos at{c.spawn_point.x + static_cast<std::int32_t>(sid % 12) * 6, c.spawn_point.y + static_cast<std::int32_t>(sid / 12) * 6};
        REQUIRE(w.spawn_player(sid, role(100 + sid, "P" + std::to_string(sid), at), id, p) == jx::pb::RESULT_OK);
    }
    for (int i = 0; i < 30; ++i) w.tick();
    w.take_outbox();

    for (std::uint64_t sid = 1; sid <= players; ++sid) REQUIRE(w.chat(sid, "xin chao"));
    std::uint64_t deliveries = 0;
    for (const Packet& p : w.take_outbox()) {
        if (p.msg_id == static_cast<std::uint16_t>(jx::pb::G2C_CHAT_MSG)) deliveries += p.sids.size();
    }
    CHECK(deliveries >= players);                                         // everybody hears at least himself
    CHECK(deliveries <= players * static_cast<std::uint64_t>(c.max_viewers + 1));
    CHECK(w.viewers_capped() > 0);
}

TEST_CASE("a newcomer in a packed place is told about it a piece at a time", "[world][aoi][interest]")
{
    // N4: a player walking into a crowd used to receive - and cause - everything in one tick.
    Quiet q;
    KSubWorldConfig c = crowd_world(0);   // no cap on players: this is about the budget alone
    c.max_known_npcs = 1000;
    KSubWorld w(c);
    for (int i = 0; i < 300; ++i) {
        w.spawn_npc("N" + std::to_string(i), Pos{c.spawn_point.x + (i % 20) * 10, c.spawn_point.y + (i / 20) * 10}, 0, 0);
    }
    EntityId id;
    Pos p;
    REQUIRE(w.spawn_player(1, role(101, "A", c.spawn_point), id, p) == jx::pb::RESULT_OK);

    std::size_t first = 0, total = 0;
    int ticks = 0;
    for (; ticks < 40 && total < 301; ++ticks) {
        std::size_t now = 0;
        for (const Packet& pk : w.take_outbox()) {
            if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_SPAWN)) continue;
            jx::pb::EntitySpawn m;
            REQUIRE(m.ParseFromString(pk.payload));
            now += static_cast<std::size_t>(m.entities_size());
            if (total == 0 && now > 0) CHECK(m.entities(0).entity_id() == id.value);   // himself first
        }
        if (ticks == 0) first = now;
        total += now;
        w.tick();
    }
    CHECK(first > 0);
    CHECK(first <= 64);          // one budget, not 301 entities in the very first packet burst
    CHECK(total == 301);         // but all of it arrives
    CHECK(ticks <= 12);          // within about half a second
}

// Hidden benchmark (run with "[.bench]"): the cost of a packed place without any network in the
// way.  3000 players on one spot is the case that broke M6; the interest pass must fit the tick.
TEST_CASE("bench: 3000 players on one spot", "[.bench]")
{
    Quiet q;
    KSubWorldConfig c = crowd_world(100);
    c.max_players = 5000;
    KSubWorld w(c);
    std::minstd_rand rng(7);
    const auto rnd = [&](int n) { return static_cast<std::int32_t>(rng() % static_cast<std::uint32_t>(n)); };
    const std::uint64_t players = 3000;
    std::uint64_t join_deliveries = 0;
    double join_worst_ms = 0;
    for (std::uint64_t sid = 1; sid <= players; ++sid) {
        EntityId id;
        Pos p;
        const Pos at{c.spawn_point.x + rnd(1200) - 600, c.spawn_point.y + rnd(1200) - 600};
        REQUIRE(w.spawn_player(sid, role(100 + sid, "P" + std::to_string(sid), at), id, p) == jx::pb::RESULT_OK);
        if (sid % 6 == 0) {   // about 100 joins a second at 18 Hz, like the load test's ramp
            const auto t0 = std::chrono::steady_clock::now();
            w.tick();
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            join_worst_ms = std::max(join_worst_ms, ms);
            for (const Packet& pk : w.take_outbox()) join_deliveries += pk.sids.size();
        }
    }
    const jx::core::TickPhase phases[] = {jx::core::TickPhase::spatial_update, jx::core::TickPhase::ai,
                                          jx::core::TickPhase::interest, jx::core::TickPhase::snapshot};
    for (const auto ph : phases) w.profile().phase_timing(ph).reset();
    double total_ms = 0, worst_ms = 0;
    std::uint64_t deliveries = 0;
    const int ticks = 180;
    for (int t = 0; t < ticks; ++t) {
        for (int k = 0; k < 170; ++k) {   // ~1 move per player per second
            const std::uint64_t sid = 1 + static_cast<std::uint64_t>(rnd(static_cast<int>(players)));
            w.move_request(sid, Pos{c.spawn_point.x + rnd(1200) - 600, c.spawn_point.y + rnd(1200) - 600}, static_cast<std::uint32_t>(t));
        }
        const auto t0 = std::chrono::steady_clock::now();
        w.tick();
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        total_ms += ms;
        worst_ms = std::max(worst_ms, ms);
        for (const Packet& pk : w.take_outbox()) deliveries += pk.sids.size();
    }
    for (const auto ph : phases) {
        const auto snap = w.profile().phase_timing(ph).snapshot();
        WARN("phase " << static_cast<int>(ph) << ": avg " << snap.avg_ms << " ms, max " << snap.max_ms << " ms");
    }
    WARN("joining: worst tick " << join_worst_ms << " ms, " << join_deliveries << " deliveries in all");
    WARN("steady: tick avg " << total_ms / ticks << " ms, worst " << worst_ms << " ms, " << deliveries / ticks
                             << " deliveries per tick, capped " << w.viewers_capped());
    CHECK(worst_ms < 55.0);
}
