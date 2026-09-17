#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "jx/frame.hpp"

using jx::frame::ParseStatus;
using jx::frame::Parser;
using jx::frame::View;

namespace {

std::vector<std::uint8_t> bytes(std::initializer_list<int> v)
{
    std::vector<std::uint8_t> out;
    for (int b : v) out.push_back(static_cast<std::uint8_t>(b));
    return out;
}

} // namespace

// Contract vector shared with Go (pkg/frame/frame_test.go) and GDScript (tests/test_net.gd).
TEST_CASE("frame encoding is little-endian len/msg/flags/payload", "[frame]")
{
    const auto f = jx::frame::encode(1001, std::string_view("hi"));
    CHECK(f == bytes({0x06, 0x00, 0x00, 0x00, 0xE9, 0x03, 0x00, 0x00, 'h', 'i'}));

    const auto empty = jx::frame::encode(0x1234, std::string_view(""), 0x0003);
    CHECK(empty == bytes({0x04, 0x00, 0x00, 0x00, 0x34, 0x12, 0x03, 0x00}));
}

TEST_CASE("parser returns frames from a stream split at arbitrary points", "[frame]")
{
    std::vector<std::uint8_t> stream;
    jx::frame::encode(stream, 1, std::string_view("one"));
    jx::frame::encode(stream, 2, std::string_view(""));
    jx::frame::encode(stream, 3, std::string_view("three"), 0x0001);

    for (std::size_t chunk = 1; chunk <= stream.size(); ++chunk) {
        Parser parser;
        std::vector<std::pair<std::uint16_t, std::string>> got;
        std::size_t pos = 0;
        while (pos < stream.size()) {
            const std::size_t n = std::min(chunk, stream.size() - pos);
            parser.feed(std::span<const std::uint8_t>(stream.data() + pos, n));
            pos += n;
            View v;
            while (parser.next(v) == ParseStatus::ok) {
                got.emplace_back(v.msg_id, std::string(v.payload.begin(), v.payload.end()));
                if (v.msg_id == 3) CHECK(v.flags == 0x0001);
            }
        }
        REQUIRE(got.size() == 3);
        CHECK(got[0] == std::make_pair(std::uint16_t{1}, std::string("one")));
        CHECK(got[1] == std::make_pair(std::uint16_t{2}, std::string("")));
        CHECK(got[2] == std::make_pair(std::uint16_t{3}, std::string("three")));
        CHECK(parser.buffered() == 0);
    }
}

TEST_CASE("parser rejects oversized and corrupt frames without consuming", "[frame]")
{
    Parser small(16);
    const auto big = jx::frame::encode(7, std::string(17, 'x'));
    small.feed(big);
    View v;
    CHECK(small.next(v) == ParseStatus::too_large);

    Parser p;
    p.feed(bytes({0x02, 0x00, 0x00, 0x00, 0x00, 0x00}));   // len 2 < header size
    CHECK(p.next(v) == ParseStatus::corrupt);

    Parser partial;
    partial.feed(bytes({0x06, 0x00, 0x00}));
    CHECK(partial.next(v) == ParseStatus::need_more);
    CHECK(partial.buffered() == 3);
    partial.reset();
    CHECK(partial.buffered() == 0);
}

TEST_CASE("parser compacts consumed bytes on the next feed", "[frame]")
{
    Parser p;
    for (int i = 0; i < 1000; ++i) {
        p.feed(jx::frame::encode(static_cast<std::uint16_t>(i), std::string_view("payload")));
        View v;
        REQUIRE(p.next(v) == ParseStatus::ok);
        CHECK(v.msg_id == static_cast<std::uint16_t>(i));
        CHECK(p.buffered() == 0);
    }
}

// Robustness: arbitrary bytes, fed in arbitrary chunks, may only produce a frame, "need more"
// or an error - never a crash, never a payload larger than the limit.  The old server cast the
// socket buffer to a packet struct, so a wrong length byte read past the buffer; this is the
// test that keeps the new parser honest.  (Go: pkg/frame/frame_fuzz_test.go, GDScript:
// tests/run.gd test_frame_fuzz.)
TEST_CASE("parser survives arbitrary garbage in arbitrary chunks", "[frame][fuzz]")
{
    std::mt19937 rng(20260917);   // fixed seed: a failure is reproducible
    constexpr std::uint32_t kLimit = 4096;
    for (int round = 0; round < 500; ++round) {
        std::vector<std::uint8_t> data;
        const int n = static_cast<int>(rng() % 600);
        data.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) data.push_back(static_cast<std::uint8_t>(rng() & 0xFFu));
        // every other round starts with a valid frame so the parser also has something to find
        if (round % 2 == 0) {
            const auto good = jx::frame::encode(static_cast<std::uint16_t>(rng() % 9000), std::string_view("hello"));
            data.insert(data.begin(), good.begin(), good.end());
        }

        Parser p(kLimit);
        const std::size_t chunk = 1 + rng() % 17;
        bool broken = false;
        for (std::size_t off = 0; off < data.size() && !broken; off += chunk) {
            const std::size_t end = std::min(off + chunk, data.size());
            p.feed(std::span<const std::uint8_t>(data.data() + off, end - off));
            for (;;) {
                View v;
                const ParseStatus st = p.next(v);
                if (st == ParseStatus::need_more) break;
                if (st != ParseStatus::ok) {   // corrupt / too_large: the connection would close
                    broken = true;
                    break;
                }
                REQUIRE(v.payload.size() <= kLimit);
            }
        }
        if (!broken) CHECK(p.buffered() <= data.size());
    }
}
