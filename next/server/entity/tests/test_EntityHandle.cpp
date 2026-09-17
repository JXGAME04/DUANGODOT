// EntityHandle: index + generation packed into the id that travels on the wire (SPEC 5).
#include <catch2/catch_test_macros.hpp>

#include "jx/entity/EntityHandle.h"

using jx::EntityId;
using jx::entity::generation_of;
using jx::entity::index_of;
using jx::entity::is_handle;
using jx::entity::make_handle;
using jx::entity::next_generation;
using jx::entity::to_string;

TEST_CASE("a handle packs an index and a generation", "[entity][handle]")
{
    const EntityId h = make_handle(500, 12);
    CHECK(index_of(h) == 500);
    CHECK(generation_of(h) == 12);
    CHECK(is_handle(h));
    CHECK(to_string(h) == "500:12");
    CHECK(h.valid());

    // the example of the spec: the same index with the next generation is a different handle
    const EntityId after_death = make_handle(500, 13);
    CHECK(after_death != h);
    CHECK(index_of(after_death) == index_of(h));
}

TEST_CASE("the empty id stays none", "[entity][handle]")
{
    const EntityId none;
    CHECK(none.value == 0);
    CHECK_FALSE(is_handle(none));
    CHECK_FALSE(none.valid());
    CHECK(to_string(none) == "none");
    // slot 0 with generation 1 is a real handle and must not look like "none"
    CHECK(make_handle(0, 1).value != 0);
    CHECK(is_handle(make_handle(0, 1)));
}

TEST_CASE("generations wrap without ever hitting zero", "[entity][handle]")
{
    CHECK(next_generation(1) == 2);
    CHECK(next_generation(0xFFFFFFFEu) == 0xFFFFFFFFu);
    CHECK(next_generation(0xFFFFFFFFu) == 1u);   // wraps past 0, which means "none"
}

TEST_CASE("the whole range of indexes and generations round trips", "[entity][handle]")
{
    for (std::uint32_t index : {0u, 1u, 1000u, 0x7FFFFFFFu, 0xFFFFFFFEu}) {
        for (std::uint32_t gen : {1u, 2u, 0xFFFFu, 0xFFFFFFFFu}) {
            const EntityId h = make_handle(index, gen);
            CHECK(index_of(h) == index);
            CHECK(generation_of(h) == gen);
        }
    }
}
