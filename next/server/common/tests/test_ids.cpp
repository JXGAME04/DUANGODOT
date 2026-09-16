#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

#include "jx/ids.hpp"

TEST_CASE("ids are distinct types with a zero none value", "[ids]")
{
    STATIC_REQUIRE_FALSE(std::is_convertible_v<jx::SessionId, jx::PlayerId>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<std::uint64_t, jx::PlayerId>);

    const jx::PlayerId none;
    const jx::PlayerId p1{1};
    const jx::PlayerId p2{2};
    CHECK_FALSE(none.valid());
    CHECK_FALSE(static_cast<bool>(none));
    CHECK(p1.valid());
    CHECK(p1 != p2);
    CHECK(p1 < p2);
    CHECK(p1 == jx::PlayerId{1});
    CHECK(jx::to_string(p2) == "2");
    CHECK(fmt::format("{}", p2) == "2");
    CHECK(fmt::format("{:>4}", p2) == "   2");
}

TEST_CASE("ids work as unordered keys", "[ids]")
{
    std::unordered_map<jx::EntityId, int, jx::IdHash> hp;
    hp[jx::EntityId{10}] = 100;
    hp[jx::EntityId{11}] = 50;
    CHECK(hp.at(jx::EntityId{10}) == 100);
    CHECK(hp.size() == 2);
}

TEST_CASE("generator never returns zero and is unique across threads", "[ids]")
{
    jx::IdGenerator gen;
    CHECK(gen.next<jx::SessionId>() == jx::SessionId{1});
    CHECK(gen.next<jx::SessionId>() == jx::SessionId{2});
    CHECK(gen.peek() == 3);

    jx::IdGenerator zero_start(0);
    CHECK(zero_start.next<jx::EntityId>() == jx::EntityId{1});

    jx::IdGenerator shared(jx::seed_from_time());
    constexpr int kThreads = 4;
    constexpr int kPerThread = 1000;
    std::vector<std::vector<jx::EntityId>> got(kThreads);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            auto& mine = got[static_cast<std::size_t>(t)];
            mine.reserve(kPerThread);
            for (int i = 0; i < kPerThread; ++i) mine.push_back(shared.next<jx::EntityId>());
        });
    }
    for (auto& th : threads) th.join();

    std::unordered_set<jx::EntityId, jx::IdHash> all;
    for (const auto& v : got) {
        for (const auto id : v) {
            CHECK(id.valid());
            all.insert(id);
        }
    }
    CHECK(all.size() == static_cast<std::size_t>(kThreads * kPerThread));
}

TEST_CASE("time seed is large and increasing", "[ids]")
{
    const auto a = jx::seed_from_time();
    CHECK(a > (std::uint64_t{1} << 40));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    CHECK(jx::seed_from_time() > a);
}
