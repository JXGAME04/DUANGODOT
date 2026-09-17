#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/net/connection.hpp"
#include "jx/net/proto.hpp"
#include "jx/client.pb.h"
#include "jx/msg.pb.h"

using namespace std::chrono_literals;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::trace;
        jx::log::init(o);
    }
};

// Runs the io_context until pred() is true or the deadline passes.
template <class Pred>
bool run_until(asio::io_context& io, Pred pred, std::chrono::milliseconds deadline = 5000ms)
{
    const auto end = std::chrono::steady_clock::now() + deadline;
    while (!pred() && std::chrono::steady_clock::now() < end) {
        io.run_for(10ms);
        if (io.stopped()) io.restart();
    }
    return pred();
}

} // namespace

TEST_CASE("listener accepts, frames round trip both ways and close is reported once", "[net]")
{
    Quiet quiet;
    asio::io_context io;
    jx::net::Listener listener(io, jx::frame::kMaxClientPayload);
    REQUIRE(!listener.open("127.0.0.1", 0));
    const std::uint16_t port = listener.port();
    REQUIRE(port != 0);

    jx::net::Connection::Ptr server_side;
    std::vector<std::string> server_got;
    int server_closes = 0;
    listener.start([&](jx::net::Connection::Ptr c) {
        server_side = c;
        c->start(
            [&](jx::net::Connection& conn, const jx::frame::View& v) {
                server_got.emplace_back(v.payload.begin(), v.payload.end());
                conn.send(static_cast<std::uint16_t>(v.msg_id + 1000), v.payload);   // echo with id+1000
            },
            [&](jx::net::Connection&, const std::error_code&) { ++server_closes; });
    });

    jx::net::Connection::Ptr client;
    std::vector<std::pair<std::uint16_t, std::string>> client_got;
    int client_closes = 0;
    jx::net::connect(io, "127.0.0.1", port, jx::frame::kMaxClientPayload, [&](const std::error_code& ec, jx::net::Connection::Ptr c) {
        REQUIRE(!ec);
        client = c;
        client->start(
            [&](jx::net::Connection&, const jx::frame::View& v) {
                client_got.emplace_back(v.msg_id, std::string(v.payload.begin(), v.payload.end()));
            },
            [&](jx::net::Connection&, const std::error_code&) { ++client_closes; });
        for (int i = 0; i < 50; ++i) client->send(static_cast<std::uint16_t>(i), "payload-" + std::to_string(i));
    });

    REQUIRE(run_until(io, [&] { return client_got.size() == 50; }));
    REQUIRE(server_got.size() == 50);
    for (int i = 0; i < 50; ++i) {
        CHECK(client_got[static_cast<std::size_t>(i)].first == static_cast<std::uint16_t>(i + 1000));
        CHECK(client_got[static_cast<std::size_t>(i)].second == "payload-" + std::to_string(i));
    }
    CHECK(server_side->frames_in == 50);
    CHECK(client->frames_out == 50);

    client->close();
    REQUIRE(run_until(io, [&] { return server_closes == 1 && !client->is_open(); }));
    CHECK(server_closes == 1);
    CHECK(!server_side->is_open());
    listener.stop();
    io.run_for(50ms);
    CHECK(server_closes == 1);
}

TEST_CASE("protobuf messages travel inside frames", "[net]")
{
    Quiet quiet;
    asio::io_context io;
    jx::net::Listener listener(io, jx::frame::kMaxClientPayload);
    REQUIRE(!listener.open("127.0.0.1", 0));

    jx::pb::LoginRes received;
    bool got = false;
    jx::net::Connection::Ptr server_side;
    listener.start([&](jx::net::Connection::Ptr c) {
        server_side = c;
        c->start(
            [&](jx::net::Connection& conn, const jx::frame::View& v) {
                REQUIRE(v.msg_id == jx::pb::C2G_LOGIN);
                jx::pb::LoginReq req;
                REQUIRE(jx::net::parse(v, req));
                jx::pb::LoginRes res;
                res.set_result(jx::pb::RESULT_OK);
                res.set_account_id(77);
                res.set_text("welcome " + req.account());
                jx::net::send(conn, jx::pb::G2C_LOGIN_RES, res);
            },
            [](jx::net::Connection&, const std::error_code&) {});
    });

    jx::net::Connection::Ptr client;
    jx::net::connect(io, "127.0.0.1", listener.port(), jx::frame::kMaxClientPayload, [&](const std::error_code& ec, jx::net::Connection::Ptr c) {
        REQUIRE(!ec);
        client = c;
        client->start(
            [&](jx::net::Connection&, const jx::frame::View& v) {
                REQUIRE(v.msg_id == jx::pb::G2C_LOGIN_RES);
                REQUIRE(jx::net::parse(v, received));
                got = true;
            },
            [](jx::net::Connection&, const std::error_code&) {});
        jx::pb::LoginReq req;
        req.set_account("tester");
        req.set_password("x");
        jx::net::send(*client, jx::pb::C2G_LOGIN, req);
    });

    REQUIRE(run_until(io, [&] { return got; }));
    CHECK(received.result() == jx::pb::RESULT_OK);
    CHECK(received.account_id() == 77);
    CHECK(received.text() == "welcome tester");
    client->close();
    server_side->close();
    io.run_for(50ms);
}

TEST_CASE("oversized frame closes the connection with an error", "[net]")
{
    Quiet quiet;
    asio::io_context io;
    jx::net::Listener listener(io, 32);   // server accepts at most 32-byte payloads
    REQUIRE(!listener.open("127.0.0.1", 0));

    std::error_code close_ec;
    bool closed = false;
    jx::net::Connection::Ptr server_side;
    listener.start([&](jx::net::Connection::Ptr c) {
        server_side = c;
        c->start([](jx::net::Connection&, const jx::frame::View&) {},
                 [&](jx::net::Connection&, const std::error_code& ec) { closed = true; close_ec = ec; });
    });

    jx::net::Connection::Ptr client;
    jx::net::connect(io, "127.0.0.1", listener.port(), jx::frame::kMaxClientPayload, [&](const std::error_code& ec, jx::net::Connection::Ptr c) {
        REQUIRE(!ec);
        client = c;
        client->start([](jx::net::Connection&, const jx::frame::View&) {}, [](jx::net::Connection&, const std::error_code&) {});
        client->send(1, std::string(100, 'x'));
    });

    REQUIRE(run_until(io, [&] { return closed; }));
    CHECK(close_ec == std::errc::bad_message);
    CHECK(!server_side->is_open());
}
