// engine/ecs/archetype.cpp
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.3.

#include "archetype.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace gw {
namespace ecs {

// ---------------------------------------------------------------------------
// Chunk
// ---------------------------------------------------------------------------

Chunk::Chunk(std::size_t chunk_bytes, std::uint16_t slots_per_chunk)
    : data_(std::make_unique<std::byte[]>(chunk_bytes))
    , entities_(std::make_unique<Entity[]>(slots_per_chunk))
    , capacity_(slots_per_chunk)
    , live_count_(0) {
    // Entities default-construct to Entity{0} (null) — fine.
    (void)chunk_bytes;
}

// ---------------------------------------------------------------------------
// Archetype
// ---------------------------------------------------------------------------

namespace {

// Round `v` up to the nearest multiple of `align` (power-of-two or not).
constexpr std::size_t align_up(std::size_t v, std::size_t align) noexcept {
    if (align <= 1) return v;
    return ((v + align - 1) / align) * align;
}

} // namespace

Archetype::Archetype(ArchetypeId                            id,
                     std::vector<ComponentTypeId>           types,
                     ComponentMask                          mask,
                     const ComponentRegistry&               registry,
                     std::size_t                            chunk_bytes)
    : id_(id)
    , types_(std::move(types))
    , mask_(mask)
    , offsets_(kMaxComponentTypes, std::numeric_limits<std::size_t>::max())
    , chunk_bytes_(chunk_bytes) {

    // Lay out per-component arrays back-to-back, respecting alignment.
    // Pre-sort `types_` by descending alignment to reduce internal padding.
    std::vector<ComponentTypeId> layout_order = types_;
    std::sort(layout_order.begin(), layout_order.end(),
              [&](ComponentTypeId a, ComponentTypeId b) {
                  return registry.info(a).alignment > registry.info(b).alignment;
              });

    // Compute `slots_per_chunk` as the max N such that:
    //   sum_over_types( align_up(sum_so_far, align_i) + size_i * N ) <= chunk_bytes
    // Since all arrays scale linearly with N and padding scales with alignments,
    // solve iteratively: guess N, compute required bytes, adjust.
    std::size_t per_entity_bytes = 0;
    for (auto tid : types_) {
        per_entity_bytes += registry.info(tid).size;
    }

    std::uint16_t guess = 0;
    if (per_entity_bytes == 0) {
        // Tag-only archetype (entity id only, no components). Still want 1+ slots.
        guess = static_cast<std::uint16_t>(std::min<std::size_t>(
            std::numeric_limits<std::uint16_t>::max(),
            chunk_bytes / sizeof(Entity)));
        if (guess == 0) guess = 1;
    } else {
        // Approximate: ignore inter-array padding for initial guess.
        std::size_t approx = chunk_bytes / per_entity_bytes;
        if (approx == 0) approx = 1;
        if (approx > std::numeric_limits<std::uint16_t>::max()) {
            approx = std::numeric_limits<std::uint16_t>::max();
        }
        guess = static_cast<std::uint16_t>(approx);

        // Tighten: compute actual bytes for N=guess, back off until it fits.
        auto required_bytes_for = [&](std::uint16_t N) -> std::size_t {
            std::size_t cursor = 0;
            for (auto tid : layout_order) {
                const auto& info = registry.info(tid);
                cursor = align_up(cursor, info.alignment);
                cursor += info.size * N;
            }
            return cursor;
        };
        while (guess > 1 && required_bytes_for(guess) > chunk_bytes) {
            --guess;
        }
        if (guess == 1 && required_bytes_for(1) > chunk_bytes) {
            // Single entity won't fit even in one chunk; this is a bug in
            // component sizing or chunk-bytes, not a runtime condition.
            throw std::runtime_error(
                "Archetype: per-entity footprint exceeds chunk_bytes; bump "
                "kDefaultChunkBytes or shrink the component set");
        }
    }
    slots_per_chunk_ = guess;

    // Record per-component byte offsets using the layout order.
    std::size_t cursor = 0;
    for (auto tid : layout_order) {
        const auto& info = registry.info(tid);
        cursor          = align_up(cursor, info.alignment);
        offsets_[tid]   = cursor;
        cursor         += info.size * slots_per_chunk_;
    }
}

std::size_t Archetype::offset_of(ComponentTypeId tid) const noexcept {
    if (tid >= kMaxComponentTypes) return std::numeric_limits<std::size_t>::max();
    return offsets_[tid];
}

std::byte* Archetype::component_ptr(const ComponentRegistry& registry,
                                     ComponentTypeId tid,
                                     std::uint16_t chunk_index,
                                     std::uint16_t slot) noexcept {
    if (!contains(tid)) return nullptr;
    return chunks_[chunk_index]->component_ptr(offsets_[tid],
                                                 registry.info(tid).size,
                                                 slot);
}

const std::byte* Archetype::component_ptr(const ComponentRegistry& registry,
                                            ComponentTypeId tid,
                                            std::uint16_t chunk_index,
                                            std::uint16_t slot) const noexcept {
    if (!contains(tid)) return nullptr;
    return chunks_[chunk_index]->component_ptr(offsets_[tid],
                                                 registry.info(tid).size,
                                                 slot);
}

Archetype::Slot Archetype::allocate_slot() {
    // Find the first non-full chunk; if none, grow.
    for (std::uint16_t i = 0; i < chunks_.size(); ++i) {
        if (!chunks_[i]->is_full()) {
            const auto slot = chunks_[i]->push_slot();
            return {i, slot};
        }
    }
    chunks_.emplace_back(std::make_unique<Chunk>(chunk_bytes_, slots_per_chunk_));
    const auto ci   = static_cast<std::uint16_t>(chunks_.size() - 1);
    const auto slot = chunks_.back()->push_slot();
    return {ci, slot};
}

Archetype::Tail Archetype::last_live_slot_() const noexcept {
    for (std::size_t i = chunks_.size(); i > 0; --i) {
        const auto idx = static_cast<std::uint16_t>(i - 1);
        const auto lc  = chunks_[idx]->live_count();
        if (lc > 0) {
            return {idx, static_cast<std::uint16_t>(lc - 1)};
        }
    }
    return {0, std::numeric_limits<std::uint16_t>::max()};
}

void Archetype::destruct_components_at(const ComponentRegistry& registry,
                                        std::uint16_t chunk_index,
                                        std::uint16_t slot) noexcept {
    for (auto tid : types_) {
        const auto& info = registry.info(tid);
        if (info.trivially_copyable) continue;   // no destructor needed
        auto* p = chunks_[chunk_index]->component_ptr(offsets_[tid], info.size, slot);
        if (info.destruct) info.destruct(p);
    }
}

void Archetype::move_components_between_(const ComponentRegistry& registry,
                                           Chunk& dst_chunk, std::uint16_t dst_slot,
                                           Chunk& src_chunk, std::uint16_t src_slot) noexcept {
    for (auto tid : types_) {
        const auto&   info = registry.info(tid);
        auto*         dst  = dst_chunk.component_ptr(offsets_[tid], info.size, dst_slot);
        auto*         src  = src_chunk.component_ptr(offsets_[tid], info.size, src_slot);
        if (info.trivially_copyable) {
            std::memcpy(dst, src, info.size);
        } else {
            info.move_construct(dst, src);
            if (info.destruct) info.destruct(src);
        }
    }
}

Entity Archetype::free_slot_swap_back(const ComponentRegistry& registry,
                                       std::uint16_t chunk_index,
                                       std::uint16_t slot) noexcept {
    // Locate the last live slot across all chunks. If it's the same slot we
    // just vacated, just pop and we're done.
    Tail tail = last_live_slot_();
    if (tail.slot == std::numeric_limits<std::uint16_t>::max()) {
        // No live slots — shouldn't happen in normal flow.
        return Entity::null();
    }
    if (tail.chunk_index == chunk_index && tail.slot == slot) {
        chunks_[chunk_index]->pop_slot();
        return Entity::null();
    }

    Chunk& dst_chunk = *chunks_[chunk_index];
    Chunk& src_chunk = *chunks_[tail.chunk_index];

    // Move (or memcpy for trivial) components from the last live slot into
    // the vacated slot.
    move_components_between_(registry, dst_chunk, slot, src_chunk, tail.slot);

    // Move the Entity handle too.
    const Entity swapped_entity       = src_chunk.entity_at(tail.slot);
    dst_chunk.entity_at(slot)         = swapped_entity;
    src_chunk.entity_at(tail.slot)    = Entity::null();

    src_chunk.pop_slot();
    return swapped_entity;
}

// ---------------------------------------------------------------------------
// ArchetypeTable
// ---------------------------------------------------------------------------

std::size_t ArchetypeTable::MaskHash::operator()(const ComponentMask& m) const noexcept {
    // Hash the bitset by slicing into 64-bit words. std::bitset doesn't
    // expose its buffer, so convert to_string once — acceptable here since
    // archetype creation is rare (not in any hot path).
    return std::hash<std::string>{}(m.to_string());
}

ArchetypeTable::ArchetypeTable() {
    archetypes_.reserve(32);
    by_mask_.reserve(32);
}

ArchetypeTable::~ArchetypeTable() = default;

ArchetypeTable::ArchetypeTable(ArchetypeTable&&) noexcept            = default;
ArchetypeTable& ArchetypeTable::operator=(ArchetypeTable&&) noexcept = default;

ArchetypeId ArchetypeTable::get_or_create(ComponentMask                    mask,
                                            std::vector<ComponentTypeId>    types_sorted,
                                            const ComponentRegistry&        registry) {
    if (auto it = by_mask_.find(mask); it != by_mask_.end()) {
        return it->second;
    }
    const auto id = static_cast<ArchetypeId>(archetypes_.size());
    archetypes_.emplace_back(std::make_unique<Archetype>(id, std::move(types_sorted),
                                                           mask, registry));
    by_mask_.emplace(mask, id);
    return id;
}

} // namespace ecs
} // namespace gw
