// KGameServer: accepts gateway links, drives the worlds (one KSubWorld per map) at a fixed tick
// and relays packets.  Single threaded: everything runs on the io_context that owns the server.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "jx/clock.hpp"
#include "jx/frame.hpp"
#include "jx/net/KSocket.h"
#include "jx/zone/KSubWorld.h"

namespace jx::zone {

struct KGameServerConfig {
    std::string listen_address = "0.0.0.0";
    std::uint16_t port = 17001;
    KSubWorldConfig world;                 // the default map (and the template every entry of `worlds` copies)
    std::vector<KSubWorldConfig> worlds;   // one per hosted map (KSubWorldSet of the old server); empty = just `world`
    std::uint32_t save_interval_s = 60;
    std::uint32_t stats_interval_s = 10;
    std::uint32_t max_ticks_per_update = 5;
};

class KGameServer {
public:
    KGameServer(asio::io_context& io, KGameServerConfig cfg);

    std::error_code start();
    void stop();

    [[nodiscard]] std::uint16_t port() const { return listener_.port(); }
    // The default world (the first map): where sessions without a saved map land.
    [[nodiscard]] KSubWorld& world() noexcept { return *worlds_.front(); }
    // g_SubWorldSet.SearchWorld: the world hosting a map, nullptr when not loaded here.
    [[nodiscard]] KSubWorld* world_of_map(std::uint32_t map_id) noexcept;
    [[nodiscard]] std::size_t world_count() const noexcept { return worlds_.size(); }
    [[nodiscard]] const KGameServerConfig& config() const noexcept { return cfg_; }
    [[nodiscard]] std::size_t gateway_count() const noexcept { return gateways_.size(); }
    [[nodiscard]] std::size_t session_count() const noexcept { return session_gateway_.size(); }

private:
    struct Gateway {
        net::Connection::Ptr conn;
        std::string id;
        bool ready = false;
    };

    void on_accept(net::Connection::Ptr conn);
    void on_frame(net::Connection& conn, const frame::View& view);
    void on_close(net::Connection& conn, const std::error_code& ec);

    void handle_hello(Gateway& gw, const frame::View& view);
    void handle_session_open(Gateway& gw, const frame::View& view);
    void handle_session_close(Gateway& gw, const frame::View& view);
    void handle_client_packet(Gateway& gw, const frame::View& view);

    [[nodiscard]] KSubWorld* world_of_session(std::uint64_t sid) noexcept;
    // KNpc::ChangeWorld across worlds: the moves the trap scripts asked for during the tick
    void process_world_changes();
    void send_to_session(std::uint64_t sid, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    [[nodiscard]] std::size_t total_entities() const noexcept;

    void flush_outbox();
    void send_save(std::uint64_t sid, bool final);
    void send_stats();
    void schedule_tick();
    void run_ticks();

    asio::io_context& io_;
    KGameServerConfig cfg_;
    net::Listener listener_;
    std::vector<std::unique_ptr<KSubWorld>> worlds_;
    std::unordered_map<std::uint32_t, KSubWorld*> world_by_map_;
    std::unordered_map<std::uint64_t, KSubWorld*> session_world_;   // sid -> the world it plays in
    asio::steady_timer timer_;
    FixedStep step_;
    Nanos last_update_{0};
    std::unordered_map<std::uint64_t, Gateway> gateways_;           // connection id -> gateway
    std::unordered_map<std::uint64_t, std::uint64_t> session_gateway_; // sid -> connection id
    bool running_ = false;
    // tick statistics for the periodic zone.tick line
    double tick_ms_sum_ = 0;
    double tick_ms_max_ = 0;
    std::uint64_t tick_samples_ = 0;
    std::uint64_t last_stats_tick_ = 0;
    std::uint64_t last_save_tick_ = 0;
};

} // namespace jx::zone
