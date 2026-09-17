// KGameServer: the GameServer process of the MASTER SPEC.
//
// It accepts gateway links, owns the map instances, and drives them at a fixed tick.  The
// network thread (the io_context) never touches world state: it decodes a frame and pushes a
// command into the inbox of the map instance that owns the session (SPEC 30).  The simulation
// runs on a pool of workers, one owner per instance (SPEC 6, 7), assigned by the scheduler
// (SPEC 8, 9).  After the parallel phase the server thread collects what the worlds produced
// and sends it, so nothing but an owner ever writes into a world.
//
//   gateway frame ──> command queue ──> [worker w] instance.tick() ──> events + outbox ──> gateway
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "jx/clock.hpp"
#include "jx/core/FixedTick.h"
#include "jx/core/JobSystem.h"
#include "jx/core/ServerClock.h"
#include "jx/core/ThreadPool.h"
#include "jx/frame.hpp"
#include "jx/net/KSocket.h"
#include "jx/zone/KMapInstance.h"
#include "jx/zone/KWorldScheduler.h"

namespace jx::zone {

struct KGameServerConfig {
    std::string listen_address = "0.0.0.0";
    std::uint16_t port = 17001;
    KSubWorldConfig world;                 // the default map (and the template every entry of `worlds` copies)
    std::vector<KSubWorldConfig> worlds;   // one per hosted map (KSubWorldSet of the old server); empty = just `world`
    std::uint32_t save_interval_s = 60;
    std::uint32_t stats_interval_s = 10;
    std::uint32_t max_ticks_per_update = 5;
    // Simulation workers (SPEC 14, 15): 0 = decide from the machine and the number of maps.
    // Never hard coded, always configurable (SPEC 83).
    std::uint32_t simulation_threads = 0;
    // How often the scheduler may move a map to another worker (SPEC 8); 0 = never.
    std::uint32_t rebalance_interval_s = 15;
};

class KGameServer {
public:
    KGameServer(asio::io_context& io, KGameServerConfig cfg);
    ~KGameServer();

    std::error_code start();
    void stop();

    [[nodiscard]] std::uint16_t port() const { return listener_.port(); }
    // The default map instance (the first one): where sessions without a saved map land.
    [[nodiscard]] KSubWorld& world() noexcept { return instances_.front()->world(); }
    [[nodiscard]] KMapInstance& instance() noexcept { return *instances_.front(); }
    // g_SubWorldSet.SearchWorld: the instance hosting a map, nullptr when not loaded here.
    [[nodiscard]] KMapInstance* instance_of_map(std::uint32_t map_id) noexcept;
    [[nodiscard]] KSubWorld* world_of_map(std::uint32_t map_id) noexcept;
    [[nodiscard]] std::size_t world_count() const noexcept { return instances_.size(); }
    [[nodiscard]] const KGameServerConfig& config() const noexcept { return cfg_; }
    [[nodiscard]] std::size_t gateway_count() const noexcept { return gateways_.size(); }
    [[nodiscard]] std::size_t session_count() const noexcept { return session_instance_.size(); }
    [[nodiscard]] unsigned simulation_workers() const noexcept { return workers_; }
    // What each simulation worker carries right now (tests, logs, metrics).
    [[nodiscard]] std::vector<KWorkerLoad> worker_loads() const;

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

    [[nodiscard]] KMapInstance* instance_of_session(std::uint64_t sid) noexcept;
    void bind_session(std::uint64_t sid, KMapInstance& target, std::uint64_t conn_id);
    void unbind_session(std::uint64_t sid);

    // after the parallel phase, on the server thread
    void process_events();
    void handle_event(KMapInstance& source, KWorldEvent& ev);
    void flush_outbox();
    void send_to_session(std::uint64_t sid, std::uint16_t msg_id, const google::protobuf::MessageLite& msg);
    void send_save(const KEvPlayerSave& save);
    void send_stats();
    void schedule_tick();
    void run_ticks();
    void tick_once();
    void rebuild_buckets();
    [[nodiscard]] std::size_t total_entities() const noexcept;

    asio::io_context& io_;
    KGameServerConfig cfg_;
    net::Listener listener_;
    // one instance per hosted map (SPEC 38: a map id can have several instances later)
    std::vector<std::unique_ptr<KMapInstance>> instances_;
    std::vector<KMapInstance*> instance_ptrs_;
    std::unordered_map<std::uint32_t, KMapInstance*> instance_by_map_;
    std::unordered_map<std::uint64_t, KMapInstance*> session_instance_;   // sid -> where it plays
    // simulation (SPEC 14): one pool, one job system, one scheduler
    unsigned workers_ = 1;
    std::unique_ptr<core::ThreadPool> pool_;
    std::unique_ptr<core::JobSystem> jobs_;
    KWorldScheduler scheduler_;
    std::vector<std::vector<KMapInstance*>> buckets_;   // worker -> the instances it ticks
    core::ServerClock clock_;
    core::FixedTick fixed_;
    core::TickProfile profile_;

    asio::steady_timer timer_;
    Nanos last_update_{0};
    std::unordered_map<std::uint64_t, Gateway> gateways_;               // connection id -> gateway
    std::unordered_map<std::uint64_t, std::uint64_t> session_gateway_;  // sid -> connection id
    bool running_ = false;
    std::uint64_t last_stats_tick_ = 0;
    // How far a gateway link may fall behind before the zone stops sending it positions.  4 MiB
    // is about two seconds of a busy link, which is already far more than a position is worth.
    static constexpr std::size_t kLinkBacklogLimit = 4 * 1024 * 1024;
    std::uint64_t link_dropped_ = 0;   // positions dropped because a link was behind
    std::uint64_t last_save_tick_ = 0;
    std::uint64_t last_rebalance_tick_ = 0;
};

} // namespace jx::zone
