// End-to-end test of the zone through its real TCP interface with a fake gateway.
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <functional>
#include <vector>

#include "jx/client.pb.h"
#include "jx/internal.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/net/KSocket.h"
#include "jx/net/proto.hpp"
#include "jx/zone/KGameServer.h"

using namespace std::chrono_literals;

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

struct Received {
    std::uint16_t msg_id;
    std::string payload;
};

struct FakeGateway {
    asio::io_context& io;
    jx::net::Connection::Ptr conn;
    std::vector<Received> frames;

    explicit FakeGateway(asio::io_context& ctx) : io(ctx) {}

    void connect(std::uint16_t port)
    {
        bool done = false;
        jx::net::connect(io, "127.0.0.1", port, jx::frame::kMaxInternalPayload, [&](const std::error_code& ec, jx::net::Connection::Ptr c) {
            REQUIRE(!ec);
            conn = c;
            conn->start([this](jx::net::Connection&, const jx::frame::View& v) {
                            frames.push_back({v.msg_id, std::string(v.payload.begin(), v.payload.end())});
                        },
                        [](jx::net::Connection&, const std::error_code&) {});
            done = true;
        });
        REQUIRE(run_until([&] { return done; }));
    }

    template <class Msg>
    void send(jx::pb::MsgId id, const Msg& m)
    {
        jx::net::send(*conn, static_cast<std::uint16_t>(id), m);
    }

    bool run_until(const std::function<bool()>& pred, std::chrono::milliseconds deadline = 5000ms)
    {
        const auto end = std::chrono::steady_clock::now() + deadline;
        while (!pred() && std::chrono::steady_clock::now() < end) {
            io.run_for(5ms);
            if (io.stopped()) io.restart();
        }
        return pred();
    }

    std::size_t count(jx::pb::MsgId id) const
    {
        std::size_t n = 0;
        for (const auto& f : frames) n += (f.msg_id == id);
        return n;
    }

    template <class Msg>
    std::vector<Msg> all(jx::pb::MsgId id) const
    {
        std::vector<Msg> out;
        for (const auto& f : frames) {
            if (f.msg_id != id) continue;
            Msg m;
            REQUIRE(m.ParseFromString(f.payload));
            out.push_back(m);
        }
        return out;
    }

    // ZonePackets addressed to sid carrying inner message 'inner'
    std::vector<jx::pb::ZonePacket> zone_packets(std::uint64_t sid, jx::pb::MsgId inner) const
    {
        std::vector<jx::pb::ZonePacket> out;
        for (const auto& zp : all<jx::pb::ZonePacket>(jx::pb::ZG_ZONE_PACKET)) {
            if (zp.msg_id() != static_cast<std::uint32_t>(inner)) continue;
            for (const auto s : zp.sids()) {
                if (s == sid) {
                    out.push_back(zp);
                    break;
                }
            }
        }
        return out;
    }
};

jx::pb::RoleData role(std::uint64_t pid, const char* name)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(1);
    return r;
}

} // namespace

TEST_CASE("gateway handshake, sessions, movement over ticks, save on close", "[zone][net]")
{
    Quiet q;
    asio::io_context io;
    jx::zone::KGameServerConfig cfg;
    cfg.listen_address = "127.0.0.1";
    cfg.port = 0;
    cfg.world.tick_hz = 20;
    cfg.world.spawn_point = jx::zone::Pos{1000, 1000};
    cfg.world.default_speed = 200;
    cfg.stats_interval_s = 0;
    cfg.save_interval_s = 0;
    jx::zone::KGameServer server(io, cfg);
    REQUIRE(!server.start());
    REQUIRE(server.port() != 0);

    FakeGateway gw(io);
    gw.connect(server.port());

    // a frame before ZoneHello is rejected -> use a second throw-away link
    {
        FakeGateway early(io);
        early.connect(server.port());
        jx::pb::SessionOpen open;
        open.set_sid(99);
        early.send(jx::pb::GZ_SESSION_OPEN, open);
        REQUIRE(early.run_until([&] { return !early.conn->is_open(); }));
    }

    jx::pb::ZoneHello hello;
    hello.set_protocol_version(jx::pb::PROTOCOL_VERSION);
    hello.set_gateway_id("gw-test");
    gw.send(jx::pb::GZ_ZONE_HELLO, hello);
    REQUIRE(gw.run_until([&] { return gw.count(jx::pb::ZG_ZONE_HELLO_ACK) == 1; }));
    const auto ack = gw.all<jx::pb::ZoneHelloAck>(jx::pb::ZG_ZONE_HELLO_ACK)[0];
    CHECK(ack.zone_id() == cfg.world.zone_id);
    CHECK(ack.tick_hz() == 20);
    CHECK(server.gateway_count() == 1);

    // session 1 enters
    jx::pb::SessionOpen open1;
    open1.set_sid(1);
    open1.set_account_id(10);
    *open1.mutable_role() = role(11, "A");
    gw.send(jx::pb::GZ_SESSION_OPEN, open1);
    REQUIRE(gw.run_until([&] { return gw.count(jx::pb::ZG_SESSION_OPEN_ACK) == 1; }));
    const auto ack1 = gw.all<jx::pb::SessionOpenAck>(jx::pb::ZG_SESSION_OPEN_ACK)[0];
    CHECK(ack1.result() == jx::pb::RESULT_OK);
    CHECK(ack1.entity_id() != 0);
    CHECK(ack1.pos().x() == 1000);
    REQUIRE(gw.run_until([&] { return gw.zone_packets(1, jx::pb::G2C_ENTITY_SPAWN).size() == 1; }));

    // session 2 enters next to it
    jx::pb::SessionOpen open2;
    open2.set_sid(2);
    *open2.mutable_role() = role(22, "B");
    gw.send(jx::pb::GZ_SESSION_OPEN, open2);
    REQUIRE(gw.run_until([&] { return gw.count(jx::pb::ZG_SESSION_OPEN_ACK) == 2 && gw.zone_packets(2, jx::pb::G2C_ENTITY_SPAWN).size() == 1; }));
    jx::pb::EntitySpawn b_sees;
    REQUIRE(b_sees.ParseFromString(gw.zone_packets(2, jx::pb::G2C_ENTITY_SPAWN)[0].payload()));
    CHECK(b_sees.entities_size() == 2);
    REQUIRE(gw.run_until([&] { return gw.zone_packets(1, jx::pb::G2C_ENTITY_SPAWN).size() == 2; }));
    CHECK(server.session_count() == 2);

    // session 1 moves 100 units right: 10 ticks at 20 Hz = 0.5 s
    jx::pb::MoveReq mv;
    mv.mutable_target()->set_x(1100);
    mv.mutable_target()->set_y(1000);
    mv.set_seq(7);
    jx::pb::ClientPacket cp;
    cp.set_sid(1);
    cp.set_msg_id(jx::pb::C2G_MOVE);
    mv.SerializeToString(cp.mutable_payload());
    gw.send(jx::pb::GZ_CLIENT_PACKET, cp);
    REQUIRE(gw.run_until([&] { return gw.zone_packets(1, jx::pb::G2C_ENTITY_MOVE).size() >= 1 && gw.zone_packets(2, jx::pb::G2C_ENTITY_MOVE).size() >= 1; }));
    {
        jx::pb::EntityMove own, other;
        REQUIRE(own.ParseFromString(gw.zone_packets(1, jx::pb::G2C_ENTITY_MOVE)[0].payload()));
        REQUIRE(other.ParseFromString(gw.zone_packets(2, jx::pb::G2C_ENTITY_MOVE)[0].payload()));
        CHECK(own.seq() == 7);
        CHECK(other.seq() == 0);
        CHECK(own.target().x() == 1100);
    }
    // arrival is announced by the tick loop
    REQUIRE(gw.run_until([&] { return gw.zone_packets(2, jx::pb::G2C_ENTITY_MOVE).size() >= 2; }, 3000ms));
    {
        jx::pb::EntityMove arrived;
        REQUIRE(arrived.ParseFromString(gw.zone_packets(2, jx::pb::G2C_ENTITY_MOVE)[1].payload()));
        CHECK(arrived.pos().x() == 1100);
        CHECK(arrived.target().x() == 1100);
        CHECK(arrived.tick() >= 10);
    }
    CHECK(server.world().find_player(1)->pos() == jx::zone::Pos{1100, 1000});

    // chat reaches both
    jx::pb::ChatReq chat;
    chat.set_text("xin chao");
    cp.set_msg_id(jx::pb::C2G_CHAT);
    chat.SerializeToString(cp.mutable_payload());
    gw.send(jx::pb::GZ_CLIENT_PACKET, cp);
    REQUIRE(gw.run_until([&] { return gw.zone_packets(2, jx::pb::G2C_CHAT_MSG).size() == 1; }));
    CHECK(gw.zone_packets(2, jx::pb::G2C_CHAT_MSG)[0].sids_size() == 2);

    // session 1 leaves: PlayerSave(final) with the new position, B gets a despawn
    jx::pb::SessionClose close;
    close.set_sid(1);
    gw.send(jx::pb::GZ_SESSION_CLOSE, close);
    REQUIRE(gw.run_until([&] { return gw.count(jx::pb::ZG_PLAYER_SAVE) == 1 && gw.zone_packets(2, jx::pb::G2C_ENTITY_DESPAWN).size() == 1; }));
    const auto save = gw.all<jx::pb::PlayerSave>(jx::pb::ZG_PLAYER_SAVE)[0];
    CHECK(save.final());
    CHECK(save.sid() == 1);
    CHECK(save.role().player_id() == 11);
    CHECK(save.role().position().pos().x() == 1100);
    CHECK(save.role().position().zone_id() == cfg.world.zone_id);
    CHECK(server.session_count() == 1);

    // gateway drop removes the remaining player
    gw.conn->close();
    REQUIRE(gw.run_until([&] { return server.gateway_count() == 0; }));
    CHECK(server.session_count() == 0);
    CHECK(server.world().player_count() == 0);

    server.stop();
    io.run_for(50ms);
}
