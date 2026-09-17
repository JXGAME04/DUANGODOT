#include "jx/core/Result.h"

#include <fmt/format.h>

namespace jx::core {

const char* error_name(ErrorCode code) noexcept
{
    switch (code) {
    case ErrorCode::ok: return "ok";
    case ErrorCode::invalid_argument: return "invalid_argument";
    case ErrorCode::not_found: return "not_found";
    case ErrorCode::already_exists: return "already_exists";
    case ErrorCode::out_of_range: return "out_of_range";
    case ErrorCode::wrong_state: return "wrong_state";
    case ErrorCode::busy: return "busy";
    case ErrorCode::unavailable: return "unavailable";
    case ErrorCode::timeout: return "timeout";
    case ErrorCode::protocol: return "protocol";
    case ErrorCode::denied: return "denied";
    case ErrorCode::full: return "full";
    case ErrorCode::unsupported: return "unsupported";
    case ErrorCode::internal: return "internal";
    }
    return "unknown";
}

std::string Error::to_string() const
{
    if (message_.empty() || message_ == error_name(code_)) return error_name(code_);
    return fmt::format("{}: {}", error_name(code_), message_);
}

} // namespace jx::core
