// jx::entity::EntityTable - the EntityManager of the MASTER SPEC (5, 36, 47, 77).
//
// One table owns the entities of one owner (a map instance, later a region): create, look up,
// validate a handle, destroy, and iterate.  Nothing else allocates or frees an entity.
//
// Storage is sparse-dense, which gives all three properties the spec asks for:
//   * stable handles      - the sparse slot keeps the generation, so a stale handle is caught;
//   * contiguous memory   - the live entities sit next to each other in one vector, which is
//                           what movement / spatial / missile loops want (SPEC 47, 77);
//   * no fixed maximum    - the vectors grow; there is no `Npc[MAX_NPC]` (SPEC 5).
//
// Not thread safe by design: a table belongs to exactly one owner, and another thread reaches
// it through that owner's command queue (SPEC 6).
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "jx/entity/EntityHandle.h"

namespace jx::entity {

template <class T>
class EntityTable {
public:
    using value_type = T;

    EntityTable() = default;
    explicit EntityTable(std::uint32_t reserve)
    {
        slots_.reserve(reserve);
        dense_.reserve(reserve);
        dense_to_slot_.reserve(reserve);
    }

    EntityTable(const EntityTable&) = delete;
    EntityTable& operator=(const EntityTable&) = delete;
    EntityTable(EntityTable&&) noexcept = default;
    EntityTable& operator=(EntityTable&&) noexcept = default;

    // Builds an entity in place and returns its handle.
    template <class... Args>
    EntityId create(Args&&... args)
    {
        const std::uint32_t slot = take_slot();
        Slot& s = slots_[slot];
        s.dense = static_cast<std::uint32_t>(dense_.size());
        s.alive = true;
        dense_.emplace_back(std::forward<Args>(args)...);
        dense_to_slot_.push_back(slot);
        ++created_;
        return make_handle(slot, s.generation);
    }

    // Moves an existing value in and returns its handle.
    EntityId insert(T value) { return create(std::move(value)); }

    [[nodiscard]] bool alive(EntityId id) const noexcept { return slot_of(id) != kInvalidIndex; }

    [[nodiscard]] T* find(EntityId id) noexcept
    {
        const std::uint32_t slot = slot_of(id);
        return slot == kInvalidIndex ? nullptr : &dense_[slots_[slot].dense];
    }
    [[nodiscard]] const T* find(EntityId id) const noexcept
    {
        const std::uint32_t slot = slot_of(id);
        return slot == kInvalidIndex ? nullptr : &dense_[slots_[slot].dense];
    }

    // Precondition: alive(id).  Use find() when that is not certain.
    [[nodiscard]] T& at(EntityId id) noexcept
    {
        T* p = find(id);
        assert(p != nullptr && "EntityTable::at on a dead or foreign handle");
        return *p;
    }
    [[nodiscard]] const T& at(EntityId id) const noexcept
    {
        const T* p = find(id);
        assert(p != nullptr && "EntityTable::at on a dead or foreign handle");
        return *p;
    }

    // Destroys the entity and invalidates every handle to it.  Returns false for a handle that
    // was already dead (which is normal: two systems can both ask for a kill in one tick).
    bool destroy(EntityId id) noexcept
    {
        const std::uint32_t slot = slot_of(id);
        if (slot == kInvalidIndex) return false;
        Slot& s = slots_[slot];
        const std::uint32_t dense_index = s.dense;
        const std::uint32_t last = static_cast<std::uint32_t>(dense_.size() - 1u);
        if (dense_index != last) {
            dense_[dense_index] = std::move(dense_[last]);
            const std::uint32_t moved_slot = dense_to_slot_[last];
            dense_to_slot_[dense_index] = moved_slot;
            slots_[moved_slot].dense = dense_index;
        }
        dense_.pop_back();
        dense_to_slot_.pop_back();
        s.alive = false;
        s.generation = next_generation(s.generation);   // every old handle is now stale
        s.dense = free_head_;
        free_head_ = slot;
        ++destroyed_;
        return true;
    }

    void clear() noexcept
    {
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(slots_.size()); ++i) {
            if (slots_[i].alive) {
                slots_[i].alive = false;
                slots_[i].generation = next_generation(slots_[i].generation);
                slots_[i].dense = free_head_;
                free_head_ = i;
                ++destroyed_;
            }
        }
        dense_.clear();
        dense_to_slot_.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept { return dense_.size(); }
    [[nodiscard]] bool empty() const noexcept { return dense_.empty(); }
    [[nodiscard]] std::size_t slot_count() const noexcept { return slots_.size(); }
    [[nodiscard]] std::uint64_t created_count() const noexcept { return created_; }
    [[nodiscard]] std::uint64_t destroyed_count() const noexcept { return destroyed_; }

    // The handle of the entity at dense position i (0 <= i < size()).
    [[nodiscard]] EntityId handle_at(std::size_t dense_index) const noexcept
    {
        const std::uint32_t slot = dense_to_slot_[dense_index];
        return make_handle(slot, slots_[slot].generation);
    }

    // Contiguous access for the hot loops.  Positions change when an entity is destroyed, so a
    // loop that destroys must go through handles instead.
    [[nodiscard]] std::vector<T>& dense() noexcept { return dense_; }
    [[nodiscard]] const std::vector<T>& dense() const noexcept { return dense_; }

    // fn(EntityId, T&) for every live entity.  Creating or destroying inside the loop is not
    // allowed (collect the handles first - see ids()).
    template <class Fn>
    void each(Fn&& fn)
    {
        for (std::size_t i = 0; i < dense_.size(); ++i) fn(handle_at(i), dense_[i]);
    }
    template <class Fn>
    void each(Fn&& fn) const
    {
        for (std::size_t i = 0; i < dense_.size(); ++i) fn(handle_at(i), dense_[i]);
    }

    // Snapshot of the live handles: the safe way to iterate while the loop changes the table.
    void ids(std::vector<EntityId>& out) const
    {
        out.clear();
        out.reserve(dense_.size());
        for (std::size_t i = 0; i < dense_.size(); ++i) out.push_back(handle_at(i));
    }
    [[nodiscard]] std::vector<EntityId> ids() const
    {
        std::vector<EntityId> out;
        ids(out);
        return out;
    }

private:
    struct Slot {
        std::uint32_t generation = 1;              // 1.. : 0 would make the handle look like "none"
        std::uint32_t dense = kInvalidIndex;       // position in dense_, or the next free slot
        bool alive = false;
    };

    [[nodiscard]] std::uint32_t slot_of(EntityId id) const noexcept
    {
        const std::uint32_t index = index_of(id);
        if (index >= slots_.size()) return kInvalidIndex;
        const Slot& s = slots_[index];
        if (!s.alive || s.generation != generation_of(id)) return kInvalidIndex;
        return index;
    }

    std::uint32_t take_slot()
    {
        if (free_head_ != kInvalidIndex) {
            const std::uint32_t slot = free_head_;
            free_head_ = slots_[slot].dense;
            return slot;
        }
        slots_.push_back(Slot{});
        return static_cast<std::uint32_t>(slots_.size() - 1u);
    }

    std::vector<Slot> slots_;                 // sparse: one per slot ever used
    std::vector<T> dense_;                    // live entities, contiguous
    std::vector<std::uint32_t> dense_to_slot_;
    std::uint32_t free_head_ = kInvalidIndex;
    std::uint64_t created_ = 0;
    std::uint64_t destroyed_ = 0;
};

} // namespace jx::entity
