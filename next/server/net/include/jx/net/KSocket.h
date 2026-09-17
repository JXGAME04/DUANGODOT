// jx::net - framed TCP connections on standalone Asio.
//
// Threading model: every Connection/Listener belongs to exactly one asio::io_context and all
// of its methods must be called from a handler running on that context (the zone runs a single
// io_context on the main thread together with the tick timer).  Nothing here locks.
#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "jx/net/asio.hpp"
#include "jx/frame.hpp"

namespace jx::net {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using Ptr = std::shared_ptr<Connection>;
    using FrameHandler = std::function<void(Connection&, const frame::View&)>;
    using CloseHandler = std::function<void(Connection&, const std::error_code&)>;

    Connection(asio::ip::tcp::socket socket, std::uint32_t max_payload);
    ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    // Starts the read loop.  on_close is called exactly once, after the last on_frame.
    void start(FrameHandler on_frame, CloseHandler on_close);

    void send(std::uint16_t msg_id, std::span<const std::uint8_t> payload, std::uint16_t flags = 0);
    void send(std::uint16_t msg_id, std::string_view payload, std::uint16_t flags = 0)
    {
        send(msg_id, frame::as_bytes(payload), flags);
    }
    // Sends an already framed buffer (used for fan-out: encode once, send to many).
    void send_raw(std::shared_ptr<const std::vector<std::uint8_t>> framed);

    void close();                                   // graceful: flush queue then shutdown
    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] const std::string& remote() const noexcept { return remote_; }
    [[nodiscard]] std::size_t queued_bytes() const noexcept { return queued_bytes_; }

    // Statistics for the net log line / ZoneStats.
    std::uint64_t frames_in = 0, frames_out = 0, bytes_in = 0, bytes_out = 0;

private:
    void do_read();
    void do_write();
    void fail(const std::error_code& ec);

    asio::ip::tcp::socket socket_;
    std::uint64_t id_;
    std::string remote_;
    frame::Parser parser_;
    std::vector<std::uint8_t> read_buf_;
    std::deque<std::shared_ptr<const std::vector<std::uint8_t>>> write_queue_;
    std::size_t queued_bytes_ = 0;
    bool writing_ = false;
    bool open_ = true;
    bool closing_ = false;
    bool closed_reported_ = false;
    FrameHandler on_frame_;
    CloseHandler on_close_;
};

class Listener {
public:
    using AcceptHandler = std::function<void(Connection::Ptr)>;

    Listener(asio::io_context& io, std::uint32_t max_payload);
    // Binds and listens; returns the error instead of throwing so main() can log it.
    std::error_code open(std::string_view address, std::uint16_t port);
    void start(AcceptHandler on_accept);
    void stop();
    [[nodiscard]] std::uint16_t port() const;

private:
    void do_accept();
    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
    std::uint32_t max_payload_;
    AcceptHandler on_accept_;
};

// Asynchronous connect helper (gateway side / tests / bots).
using ConnectHandler = std::function<void(const std::error_code&, Connection::Ptr)>;
void connect(asio::io_context& io, std::string_view host, std::uint16_t port, std::uint32_t max_payload,
             ConnectHandler on_connected);

} // namespace jx::net
