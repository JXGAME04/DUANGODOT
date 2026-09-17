// EntityTable: create / lookup / validate / destroy, tested hard because everything above it
// trusts a handle (MASTER SPEC 36, 90 Phase B "test rất kỹ").
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "jx/entity/EntityTable.h"

using jx::EntityId;
using jx::entity::EntityTable;
using jx::entity::generation_of;
using jx::entity::index_of;

namespace {

struct Npc {
    std::string name;
    int hp = 100;
};

} // namespace

TEST_CASE("create, find and destroy", "[entity][table]")
{
    EntityTable<Npc> table;
    CHECK(table.empty());

    const EntityId a = table.create(Npc{"Heo rừng", 40});
    const EntityId b = table.create(Npc{"Nhím", 30});
    CHECK(table.size() == 2);
    REQUIRE(table.find(a) != nullptr);
    CHECK(table.find(a)->name == "Heo rừng");
    CHECK(table.at(b).hp == 30);
    CHECK(table.alive(a));
    CHECK(a != b);

    table.at(a).hp -= 15;
    CHECK(table.find(a)->hp == 25);

    CHECK(table.destroy(a));
    CHECK_FALSE(table.alive(a));
    CHECK(table.find(a) == nullptr);
    CHECK(table.size() == 1);
    CHECK_FALSE(table.destroy(a));   // destroying twice is not an error, just false
    CHECK(table.at(b).name == "Nhím");
    CHECK(table.created_count() == 2);
    CHECK(table.destroyed_count() == 1);
}

TEST_CASE("a stale handle never reaches the entity that took the slot", "[entity][table]")
{
    EntityTable<Npc> table;
    const EntityId first = table.create(Npc{"đầu tiên"});
    REQUIRE(table.destroy(first));
    const EntityId second = table.create(Npc{"thứ hai"});

    // the slot is reused, so the index is the same and the generation is not
    CHECK(index_of(second) == index_of(first));
    CHECK(generation_of(second) == generation_of(first) + 1);
    CHECK(second != first);
    CHECK(table.find(first) == nullptr);        // the old handle is dead for ever
    REQUIRE(table.find(second) != nullptr);
    CHECK(table.find(second)->name == "thứ hai");
    CHECK(table.slot_count() == 1);             // and the slot really was recycled
}

TEST_CASE("handles from another table or from nowhere are rejected", "[entity][table]")
{
    EntityTable<Npc> table;
    const EntityId real = table.create(Npc{"thật"});
    CHECK(table.find(EntityId{}) == nullptr);                       // "none"
    CHECK(table.find(jx::entity::make_handle(9999, 1)) == nullptr);  // index out of range
    CHECK(table.find(jx::entity::make_handle(index_of(real), 99)) == nullptr);   // wrong generation
    CHECK(table.find(real) != nullptr);
}

TEST_CASE("live entities stay contiguous while slots are recycled", "[entity][table]")
{
    EntityTable<int> table;
    std::vector<EntityId> ids;
    for (int i = 0; i < 10; ++i) ids.push_back(table.create(i));
    // destroy every second one: the dense array closes the gaps (swap with the last)
    for (std::size_t i = 0; i < ids.size(); i += 2) REQUIRE(table.destroy(ids[i]));
    CHECK(table.size() == 5);
    CHECK(table.dense().size() == 5);

    std::vector<int> values;
    table.each([&](EntityId id, int& v) {
        CHECK(table.alive(id));
        values.push_back(v);
    });
    std::sort(values.begin(), values.end());
    CHECK(values == std::vector<int>{1, 3, 5, 7, 9});

    // every surviving handle still finds its own value after the moves
    for (std::size_t i = 1; i < ids.size(); i += 2) {
        REQUIRE(table.find(ids[i]) != nullptr);
        CHECK(*table.find(ids[i]) == static_cast<int>(i));
    }
}

TEST_CASE("ids() is the safe way to iterate while destroying", "[entity][table]")
{
    EntityTable<int> table;
    for (int i = 0; i < 100; ++i) table.create(i);
    std::vector<EntityId> snapshot;
    table.ids(snapshot);
    CHECK(snapshot.size() == 100);
    for (EntityId id : snapshot) {
        if (*table.find(id) % 3 == 0) table.destroy(id);
    }
    CHECK(table.size() == 66);
    for (EntityId id : snapshot) {
        const int* v = table.find(id);
        if (v != nullptr) CHECK(*v % 3 != 0);
    }
}

TEST_CASE("handles stay unique over many create and destroy cycles", "[entity][table]")
{
    EntityTable<int> table;
    std::unordered_set<std::uint64_t> seen;
    std::vector<EntityId> live;
    for (int round = 0; round < 2000; ++round) {
        const EntityId id = table.create(round);
        CHECK(seen.insert(id.value).second);   // never handed out the same handle twice
        live.push_back(id);
        if (live.size() > 10) {
            table.destroy(live.front());
            live.erase(live.begin());
        }
    }
    CHECK(table.size() == live.size());
    CHECK(table.slot_count() <= 12);   // slots really are recycled, the table does not grow
}

TEST_CASE("a table holds move only values", "[entity][table]")
{
    EntityTable<std::unique_ptr<int>> table;
    const EntityId a = table.create(std::make_unique<int>(7));
    const EntityId b = table.insert(std::make_unique<int>(8));
    CHECK(**table.find(a) == 7);
    CHECK(**table.find(b) == 8);
    table.destroy(a);
    CHECK(table.find(a) == nullptr);
    CHECK(**table.find(b) == 8);
}

TEST_CASE("clear kills every handle", "[entity][table]")
{
    EntityTable<int> table;
    std::vector<EntityId> ids;
    for (int i = 0; i < 5; ++i) ids.push_back(table.create(i));
    table.clear();
    CHECK(table.empty());
    for (EntityId id : ids) CHECK(table.find(id) == nullptr);
    const EntityId fresh = table.create(42);
    CHECK(table.find(fresh) != nullptr);
    CHECK(std::find(ids.begin(), ids.end(), fresh) == ids.end());
}

TEST_CASE("a hundred thousand entities", "[entity][table]")
{
    EntityTable<int> table(100000);
    std::vector<EntityId> ids;
    ids.reserve(100000);
    for (int i = 0; i < 100000; ++i) ids.push_back(table.create(i));
    CHECK(table.size() == 100000);
    // every handle resolves to its own value
    for (int i = 0; i < 100000; i += 997) CHECK(*table.find(ids[static_cast<std::size_t>(i)]) == i);
    for (int i = 0; i < 100000; i += 2) table.destroy(ids[static_cast<std::size_t>(i)]);
    CHECK(table.size() == 50000);
    long long sum = 0;
    for (int v : table.dense()) sum += v;
    CHECK(sum == 2500000000LL);   // 1 + 3 + ... + 99999
}
