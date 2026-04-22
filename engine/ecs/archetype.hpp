#pragma once
// engine/ecs/archetype.hpp
// Archetype-of-chunks storage. See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.3.
//
// An Archetype owns N fixed-size byte chunks (default 16 KiB). Each chunk
// holds a dense SOA: per component type, a contiguous array of
// `slots_per_chunk` component instances, packed back-to-back with alignment
// padding. Entities are stored densely: indices [0, live_count) are live;
// destroy swaps the last live entity into the freed slot (O(1)).

#include "component_registry.hpp"
#include "entity.hpp"

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace gw {
namespace ecs {

using ComponentMask = std::bitset<kMaxComponentTypes>;

inline constexpr std::size_t kDefaultChunkBytes = 16u * 1024u;

// ---------------------------------------------------------------------------
// Chunk — one fixed-size block of SOA component storage.
// ---------------------------------------------------------------------------

class Chunk {
public:
    // Layout is set by the owning Archetype: `component_offsets_` [ids] gives
    // the byte offset of each component array inside the chunk.
    Chunk(std::size_t chunk_bytes, std::uint16_t slots_per_chunk);
    ~Chunk() = default;

    Chunk(const Chunk&)            = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&&) noexcept        = default;
    Chunk& operator=(Chunk&&) noexcept = default;

    [[nodiscard]] std::uint16_t live_count() const noexcept { return live_count_; }
    [[nodiscard]] std::uint16_t capacity()   const noexcept { return capacity_; }
    [[nodiscard]] bool is_full()             const noexcept { return live_count_ >= capacity_; }

    [[nodiscard]] Entity& entity_at(std::uint16_t slot) noexcept {
        return entities_[slot];
    }
    [[nodiscard]] const Entity& entity_at(std::uint16_t slot) const noexcept {
        return entities_[slot];
    }

    // Raw byte pointer to the first component of type `type_offset_bytes`,
    // at slot `slot`. Component size is supplied by the caller (Archetype).
    [[nodiscard]] std::byte* component_ptr(std::size_t type_offset_bytes,
                                            std::size_t component_size,
                                            std::uint16_t slot) noexcept {
        return data_.get() + type_offset_bytes + component_size * slot;
    }
    [[nodiscard]] const std::byte* component_ptr(std::size_t type_offset_bytes,
                                                  std::size_t component_size,
                                                  std::uint16_t slot) const noexcept {
        return data_.get() + type_offset_bytes + component_size * slot;
    }

    // Bumps the live counter; caller must have ensured capacity.
    std::uint16_t push_slot() noexcept {
        const auto slot = live_count_;
        ++live_count_;
        return slot;
    }

    // Pops the last live slot (does NOT run destructors — caller's job).
    void pop_slot() noexcept {
        if (live_count_ > 0) --live_count_;
    }

private:
    std::unique_ptr<std::byte[]>                data_;
    std::unique_ptr<Entity[]>                   entities_;
    std::uint16_t                               capacity_   = 0;
    std::uint16_t                               live_count_ = 0;
};

// ---------------------------------------------------------------------------
// Archetype — unique sorted ComponentTypeId set + owned chunks.
// ---------------------------------------------------------------------------

using ArchetypeId = std::uint32_t;
inline constexpr ArchetypeId kInvalidArchetypeId =
    std::numeric_limits<ArchetypeId>::max();

class Archetype {
public:
    // `types` must be sorted + unique.
    Archetype(ArchetypeId                            id,
              std::vector<ComponentTypeId>           types,
              ComponentMask                          mask,
              const ComponentRegistry&               registry,
              std::size_t                            chunk_bytes = kDefaultChunkBytes);

    Archetype(const Archetype&)            = delete;
    Archetype& operator=(const Archetype&) = delete;
    Archetype(Archetype&&) noexcept        = default;
    Archetype& operator=(Archetype&&) noexcept = default;

    [[nodiscard]] ArchetypeId                   id()        const noexcept { return id_; }
    [[nodiscard]] const ComponentMask&          mask()      const noexcept { return mask_; }
    [[nodiscard]] const std::vector<ComponentTypeId>& types() const noexcept { return types_; }
    [[nodiscard]] std::uint16_t                 slots_per_chunk() const noexcept { return slots_per_chunk_; }
    [[nodiscard]] std::size_t                   chunk_count()     const noexcept { return chunks_.size(); }
    [[nodiscard]] Chunk&                        chunk(std::size_t i)       noexcept { return *chunks_[i]; }
    [[nodiscard]] const Chunk&                  chunk(std::size_t i) const noexcept { return *chunks_[i]; }

    // Returns byte-offset inside a chunk for component type `tid`, or
    // std::numeric_limits<std::size_t>::max() if not present in this archetype.
    [[nodiscard]] std::size_t offset_of(ComponentTypeId tid) const noexcept;
    [[nodiscard]] bool        contains(ComponentTypeId tid) const noexcept {
        return tid < kMaxComponentTypes && mask_.test(tid);
    }

    // Allocate a new live slot for an entity. Returns {chunk_index, slot}.
    // The slot's component memory is uninitialized; caller constructs
    // components and writes the Entity handle.
    struct Slot { std::uint16_t chunk_index; std::uint16_t slot; };
    [[nodiscard]] Slot allocate_slot();

    // Destruct the component objects at (chunk_index, slot). Does NOT free
    // the slot — use free_slot_swap_back for that.
    void destruct_components_at(const ComponentRegistry& registry,
                                 std::uint16_t chunk_index,
                                 std::uint16_t slot) noexcept;

    // Swap the last live slot of the last chunk into (chunk_index, slot),
    // then pop the last slot. Returns the Entity that was swapped-in (i.e.
    // whose EntityTable row needs its {chunk_index, slot_index} updated).
    // Returns Entity::null() if no swap was needed (the removed slot was
    // already the last one).
    //
    // Pre: the components at (chunk_index, slot) have already been moved-out
    //      or destructed by the caller.
    [[nodiscard]] Entity free_slot_swap_back(const ComponentRegistry& registry,
                                              std::uint16_t chunk_index,
                                              std::uint16_t slot) noexcept;

    // Pointer to the component of type `tid` for (chunk_index, slot).
    // Returns nullptr when `tid` is not part of this archetype.
    [[nodiscard]] std::byte* component_ptr(const ComponentRegistry& registry,
                                            ComponentTypeId tid,
                                            std::uint16_t chunk_index,
                                            std::uint16_t slot) noexcept;
    [[nodiscard]] const std::byte* component_ptr(const ComponentRegistry& registry,
                                                  ComponentTypeId tid,
                                                  std::uint16_t chunk_index,
                                                  std::uint16_t slot) const noexcept;

private:
    // Returns {last_chunk_index, last_live_slot} of the last chunk that has
    // at least one live slot, or {..., UINT16_MAX} if no live slots exist.
    struct Tail { std::uint16_t chunk_index; std::uint16_t slot; };
    [[nodiscard]] Tail last_live_slot_() const noexcept;

    void move_components_between_(const ComponentRegistry& registry,
                                   Chunk& dst_chunk, std::uint16_t dst_slot,
                                   Chunk& src_chunk, std::uint16_t src_slot) noexcept;

    ArchetypeId                               id_              = kInvalidArchetypeId;
    std::vector<ComponentTypeId>              types_;                 // sorted, unique
    ComponentMask                             mask_;
    std::vector<std::size_t>                  offsets_;               // [ComponentTypeId] -> byte offset inside chunk
    std::vector<std::unique_ptr<Chunk>>       chunks_;
    std::size_t                               chunk_bytes_    = 0;
    std::uint16_t                             slots_per_chunk_= 0;
};

// ---------------------------------------------------------------------------
// ArchetypeTable — owns all archetypes; looks up by component mask.
// ---------------------------------------------------------------------------

class ArchetypeTable {
public:
    ArchetypeTable();
    ~ArchetypeTable();

    ArchetypeTable(const ArchetypeTable&)            = delete;
    ArchetypeTable& operator=(const ArchetypeTable&) = delete;
    ArchetypeTable(ArchetypeTable&&) noexcept;
    ArchetypeTable& operator=(ArchetypeTable&&) noexcept;

    // Returns the existing archetype id for `mask`, or creates one on demand.
    // `types_sorted` must be sorted+unique and consistent with `mask`.
    [[nodiscard]] ArchetypeId get_or_create(ComponentMask                    mask,
                                             std::vector<ComponentTypeId>    types_sorted,
                                             const ComponentRegistry&        registry);

    [[nodiscard]] std::size_t archetype_count() const noexcept { return archetypes_.size(); }

    [[nodiscard]] Archetype&       archetype(ArchetypeId id)       noexcept { return *archetypes_[id]; }
    [[nodiscard]] const Archetype& archetype(ArchetypeId id) const noexcept { return *archetypes_[id]; }

    // Enumerate archetype ids whose mask is a superset of `required`.
    // Callback: bool(ArchetypeId) — return false to stop early.
    template <typename Fn>
    void for_each_matching(const ComponentMask& required, Fn&& fn) const {
        for (ArchetypeId id = 0; id < archetypes_.size(); ++id) {
            const auto& m = archetypes_[id]->mask();
            if ((m & required) == required) {
                if constexpr (std::is_same_v<decltype(fn(id)), bool>) {
                    if (!fn(id)) return;
                } else {
                    fn(id);
                }
            }
        }
    }

private:
    struct MaskHash {
        std::size_t operator()(const ComponentMask& m) const noexcept;
    };

    std::vector<std::unique_ptr<Archetype>>                   archetypes_;
    // mask -> archetype id. Used to look up the destination archetype during
    // entity migration in O(1).
    std::unordered_map<ComponentMask, ArchetypeId, MaskHash>  by_mask_;
};

} // namespace ecs
} // namespace gw
