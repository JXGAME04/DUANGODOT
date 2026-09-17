// Result / Status: expected failures are values, not exceptions (MASTER SPEC 79, 80, 88).
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "jx/core/Result.h"

using jx::core::Error;
using jx::core::ErrorCode;
using jx::core::Result;
using jx::core::Status;

namespace {

Result<int> parse_level(const std::string& text)
{
    if (text.empty()) return Error(ErrorCode::invalid_argument, "empty level");
    if (text == "999") return Error(ErrorCode::out_of_range, "level 999 is above the maximum");
    return 42;
}

Status save(bool disk_full)
{
    if (disk_full) return jx::core::fail(ErrorCode::full, "no space left");
    return jx::core::ok();
}

} // namespace

TEST_CASE("a result carries either a value or a reason", "[core][result]")
{
    const auto good = parse_level("42");
    REQUIRE(good);
    CHECK(good.ok());
    CHECK(*good == 42);
    CHECK(good.value() == 42);
    CHECK(good.code() == ErrorCode::ok);
    CHECK(good.status().ok());

    const auto bad = parse_level("");
    CHECK_FALSE(bad);
    CHECK(bad.code() == ErrorCode::invalid_argument);
    CHECK(bad.error().message() == "empty level");
    CHECK(bad.value_or(7) == 7);
    CHECK_FALSE(bad.status().ok());

    const auto oor = parse_level("999");
    CHECK(oor.code() == ErrorCode::out_of_range);
    CHECK(oor.error().to_string() == "out_of_range: level 999 is above the maximum");
}

TEST_CASE("status says ok or why not", "[core][result]")
{
    CHECK(save(false).ok());
    const Status bad = save(true);
    CHECK_FALSE(bad);
    CHECK(bad.code() == ErrorCode::full);
    CHECK(bad.message() == "no space left");
    CHECK(std::string(jx::core::error_name(ErrorCode::wrong_state)) == "wrong_state");
    CHECK(Error(ErrorCode::timeout).to_string() == "timeout");
}

TEST_CASE("a result moves a value without copying it", "[core][result]")
{
    Result<std::string> r = std::string("một chuỗi dài để chắc chắn là cấp phát trên heap");
    REQUIRE(r);
    const std::string taken = std::move(r).value();
    CHECK(taken.size() > 30);
    CHECK(r->empty());   // moved from, still a valid object
}
