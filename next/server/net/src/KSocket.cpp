#include "jx/net/KSocket.h"

#include <atomic>
#include <utility>

#include "jx/log.hpp"

namespace jx::net {
namespace {

std::uint64_t next_connection_id()
{
    static std::atomic<std::uint64_t> counter{1};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

std::string endpoint_string(const asio::ip::tcp::socket& s)
{
    std::error_code ec;
    const auto ep = s.remote_endpoint(ec);
    if (ec) return "?";
    return ep.address().to_string() + ":" + std::to_string(ep.port());
}

} // namespace

Connection::Connection(asio::ip::tcp::socket socket, std::uint32_t max_payload)
    : socket_(std::move(socket)), id_(next_connection_id()), remote_(endpoint_string(socket_)),
      parser_(max_payload), read_buf_(64 * 1024)
{
    std::error_code ec;
    socket_.set_option(asio::ip::tcp::no_delay(true), ec);
}

Connection::~Connection() = default;

void Connection::start(FrameHandler on_frame, CloseHandler on_close)
{
    on_frame_ = std::move(on_frame);
    on_close_ = std::move(on_close);
    log::debug("net", "connection open", {log::kv("conn", id_), log::kv("remote", remote_)});
    do_read();
}

void Connection::do_read()
{
    auto self = shared_from_this();
    socket_.async_read_some(asio::buffer(read_buf_), [this, self](const std::error_code& ec, std::size_t n) {
        if (ec) {
            fail(ec);
            return;
        }
        bytes_in += n;
        parser_.feed(std::span<const std::uint8_t>(read_buf_.data(), n));
        frame::View view;
        for (;;) {
            const auto st = parser_.next(view);
            if (st == frame::ParseStatus::need_more) break;
            if (st != frame::ParseStatus::ok) {
                log::warn("net", st == frame::ParseStatus::too_large ? "frame too large" : "frame corrupt",
                          {log::kv("conn", id_), log::kv("remote", remote_)});
                fail(std::make_error_code(std::errc::bad_message));
                return;
            }
            ++frames_in;
            if (on_frame_) on_frame_(*this, view);
            if (!open_) return;
        }
        do_read();
    });
}

void Connection::send(std::uint16_t msg_id, std::span<const std::uint8_t> payload, std::uint16_t flags, bool urgent)
{
    auto buf = std::make_shared<std::vector<std::uint8_t>>();
    frame::encode(*buf, msg_id, payload, flags);
    send_raw(std::move(buf), urgent);
}

void Connection::send_raw(std::shared_ptr<const std::vector<std::uint8_t>> framed, bool urgent)
{
    if (!open_ || closing_) return;
    queued_bytes_ += framed->size();
    (urgent ? urgent_queue_ : write_queue_).push_back(std::move(framed));
    if (!writing_) do_write();
}

// How much one scatter/gather write may carry.  Big enough that a tick's worth of world traffic
// goes out at once, small enough that a new control packet never waits long behind it.
constexpr std::size_t kWriteBatchBytes = 256 * 1024;

void Connection::do_write()
{
    if (write_queue_.empty() && urgent_queue_.empty()) {
        writing_ = false;
        if (closing_) {
            std::error_code ec;
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            fail(std::error_code{});   // local close: report on_close exactly once
        }
        return;
    }
    writing_ = true;
    auto self = shared_from_this();
    // Write everything that is waiting in one go.  One async_write per frame means one WSASend
    // per frame, and the zone -> gateway link carries hundreds of thousands of frames a second at
    // 10 000 players; the acks a player is waiting for then queue behind all of them.  Asio takes
    // a buffer sequence, so this stays a single scatter/gather write.
    auto batch = std::make_shared<std::vector<std::shared_ptr<const std::vector<std::uint8_t>>>>();
    std::vector<asio::const_buffer> views;
    std::size_t bytes = 0;
    // the urgent queue empties first and completely: it only ever holds acks and control frames
    auto take = [&](std::deque<std::shared_ptr<const std::vector<std::uint8_t>>>& q) {
        std::size_t taken = 0;
        for (auto& item : q) {
            if (!batch->empty() && bytes + item->size() > kWriteBatchBytes) break;
            bytes += item->size();
            views.emplace_back(item->data(), item->size());
            batch->push_back(item);
            ++taken;
        }
        return taken;
    };
    const std::size_t from_urgent = take(urgent_queue_);
    const std::size_t from_bulk = take(write_queue_);
    asio::async_write(socket_, views, [this, self, batch, bytes, from_urgent, from_bulk](const std::error_code& ec, std::size_t n) {
        if (ec) {
            fail(ec);
            return;
        }
        bytes_out += n;
        frames_out += batch->size();
        queued_bytes_ -= bytes;
        urgent_queue_.erase(urgent_queue_.begin(), urgent_queue_.begin() + static_cast<std::ptrdiff_t>(from_urgent));
        write_queue_.erase(write_queue_.begin(), write_queue_.begin() + static_cast<std::ptrdiff_t>(from_bulk));
        do_write();
    });
}

void Connection::close()
{
    if (!open_ || closing_) return;
    closing_ = true;
    if (!writing_) do_write();   // shuts down immediately when nothing is queued
}

void Connection::fail(const std::error_code& ec)
{
    if (closed_reported_) return;
    closed_reported_ = true;
    open_ = false;
    std::error_code ignore;
    socket_.close(ignore);
    write_queue_.clear();
    urgent_queue_.clear();
    queued_bytes_ = 0;
    if (ec && ec != asio::error::eof && ec != asio::error::operation_aborted) {
        log::debug("net", "connection closed", {log::kv("conn", id_), log::kv("remote", remote_), log::kv("error", ec.message())});
    } else {
        log::debug("net", "connection closed", {log::kv("conn", id_), log::kv("remote", remote_)});
    }
    if (on_close_) {
        auto cb = std::move(on_close_);
        on_close_ = nullptr;
        cb(*this, ec);
    }
}

// ---- Listener ----------------------------------------------------------------------------

Listener::Listener(asio::io_context& io, std::uint32_t max_payload) : io_(io), acceptor_(io), max_payload_(max_payload) {}

std::error_code Listener::open(std::string_view address, std::uint16_t port)
{
    std::error_code ec;
    const auto addr = asio::ip::make_address(address, ec);
    if (ec) return ec;
    const asio::ip::tcp::endpoint ep(addr, port);
    acceptor_.open(ep.protocol(), ec);
    if (ec) return ec;
    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
    acceptor_.bind(ep, ec);
    if (ec) return ec;
    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    return ec;
}

void Listener::start(AcceptHandler on_accept)
{
    on_accept_ = std::move(on_accept);
    do_accept();
}

void Listener::do_accept()
{
    acceptor_.async_accept([this](const std::error_code& ec, asio::ip::tcp::socket socket) {
        if (ec) {
            if (ec != asio::error::operation_aborted) {
                log::warn("net", "accept failed", {log::kv("error", ec.message())});
                do_accept();
            }
            return;
        }
        auto conn = std::make_shared<Connection>(std::move(socket), max_payload_);
        if (on_accept_) on_accept_(std::move(conn));
        do_accept();
    });
}

void Listener::stop()
{
    std::error_code ec;
    acceptor_.close(ec);
}

std::uint16_t Listener::port() const
{
    std::error_code ec;
    const auto ep = acceptor_.local_endpoint(ec);
    return ec ? 0 : ep.port();
}

// ---- connect -----------------------------------------------------------------------------

void connect(asio::io_context& io, std::string_view host, std::uint16_t port, std::uint32_t max_payload,
             ConnectHandler on_connected)
{
    auto resolver = std::make_shared<asio::ip::tcp::resolver>(io);
    auto socket = std::make_shared<asio::ip::tcp::socket>(io);
    resolver->async_resolve(std::string(host), std::to_string(port),
        [resolver, socket, max_payload, cb = std::move(on_connected)](const std::error_code& ec,
                                                                     asio::ip::tcp::resolver::results_type results) mutable {
            if (ec) {
                cb(ec, nullptr);
                return;
            }
            asio::async_connect(*socket, results,
                [socket, max_payload, cb = std::move(cb)](const std::error_code& ec2, const asio::ip::tcp::endpoint&) mutable {
                    if (ec2) {
                        cb(ec2, nullptr);
                        return;
                    }
                    cb({}, std::make_shared<Connection>(std::move(*socket), max_payload));
                });
        });
}

} // namespace jx::net
