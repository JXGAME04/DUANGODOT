// jx::core::Result - how a function says "no" without an exception (MASTER SPEC 79, 80, 88).
//
// The server must survive a malformed packet, a broken Lua script or an invalid entity handle
// without taking the process down, and an exception crossing a worker boundary terminates the
// process.  So the rule is: expected failures are values, exceptions stay inside the boundary
// that can handle them.
//
//     Result<EntityId> e = manager.create(...);
//     if (!e) { log::warn(...); return e.error(); }
//     use(*e);
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace jx::core {

// Small, stable set: every layer maps its own failures onto these so callers can react
// without knowing who failed.  New codes are appended, never renumbered (they reach the log).
enum class ErrorCode : std::uint16_t {
    ok = 0,
    invalid_argument,   // the caller passed something impossible
    not_found,          // handle / id / name does not exist (any more)
    already_exists,
    out_of_range,       // index, position, size outside the allowed range
    wrong_state,        // right request, wrong moment (session state, entity dead ...)
    busy,               // try again later (locked account, queue full)
    unavailable,        // a dependency is down (zone link, database)
    timeout,
    protocol,           // malformed / unexpected wire data
    denied,             // authenticated but not allowed
    full,               // capacity reached (map, inventory, account slots)
    unsupported,
    internal,           // a bug on our side
};

const char* error_name(ErrorCode code) noexcept;

// An error with a human readable reason.  The reason is for the log, the code is for the code.
class Error {
public:
    Error() noexcept = default;
    Error(ErrorCode code, std::string message) noexcept : code_(code), message_(std::move(message)) {}
    explicit Error(ErrorCode code) : code_(code), message_(error_name(code)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] bool ok() const noexcept { return code_ == ErrorCode::ok; }
    [[nodiscard]] std::string to_string() const;

private:
    ErrorCode code_ = ErrorCode::ok;
    std::string message_;
};

// Result of an operation that returns nothing.
class Status {
public:
    Status() noexcept = default;                                   // ok
    Status(Error e) noexcept : error_(std::move(e)) {}             // NOLINT(google-explicit-constructor)
    Status(ErrorCode code, std::string message) : error_(Error(code, std::move(message))) {}

    [[nodiscard]] bool ok() const noexcept { return error_.ok(); }
    explicit operator bool() const noexcept { return ok(); }
    [[nodiscard]] const Error& error() const noexcept { return error_; }
    [[nodiscard]] ErrorCode code() const noexcept { return error_.code(); }
    [[nodiscard]] const std::string& message() const noexcept { return error_.message(); }

private:
    Error error_;
};

inline Status ok() noexcept { return Status{}; }
inline Status fail(ErrorCode code, std::string message) { return Status(code, std::move(message)); }

// Result of an operation that returns a value.
template <class T>
class Result {
public:
    Result(T value) : value_(std::move(value)) {}                  // NOLINT(google-explicit-constructor)
    Result(Error e) noexcept : error_(std::move(e)) {}             // NOLINT(google-explicit-constructor)
    Result(ErrorCode code, std::string message) : error_(Error(code, std::move(message))) {}
    Result(Status s) : error_(s.error()) {}                        // NOLINT(google-explicit-constructor)

    [[nodiscard]] bool ok() const noexcept { return value_.has_value(); }
    explicit operator bool() const noexcept { return ok(); }

    // Only valid when ok(); the caller checks first (debug builds assert through the tests).
    const T& value() const& noexcept { return *value_; }
    T& value() & noexcept { return *value_; }
    T&& value() && noexcept { return std::move(*value_); }
    const T& operator*() const& noexcept { return *value_; }
    T& operator*() & noexcept { return *value_; }
    const T* operator->() const noexcept { return &*value_; }
    T* operator->() noexcept { return &*value_; }

    // The value, or a fallback when the call failed: no branch at the call site.
    [[nodiscard]] T value_or(T fallback) const { return value_.has_value() ? *value_ : std::move(fallback); }

    [[nodiscard]] const Error& error() const noexcept { return error_; }
    [[nodiscard]] ErrorCode code() const noexcept { return value_.has_value() ? ErrorCode::ok : error_.code(); }
    [[nodiscard]] Status status() const { return value_.has_value() ? Status{} : Status(error_); }

private:
    std::optional<T> value_;
    Error error_;
};

} // namespace jx::core
