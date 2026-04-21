#pragma once

#include "entity.hpp"
#include "component.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace gw {
namespace ecs {

// Forward declaration
class Archetype;

// ---------------------------------------------------------------------------
// Chunk — tightly-packed SOA storage for entities with identical archetypes.
//
// Design:
//   - Each component type gets its own contiguous byte array (SOA).
//   - Entity indices are assigned from a free-list for O(1) create/destroy.
//   - Per-slot generation counter turns 32-bit EntityHandle into a 64-bit
//     conceptual (index:16 | generation:16) ABA-safe token.
//   - Maximum 65535 entities per chunk.  Phase 20 uses multiple chunks per
//     archetype when the limit is reached.
// ---------------------------------------------------------------------------
class Chunk {
public:
    // Practical per-chunk limits.  A 16 KiB L1 cache holds ~1024 transforms.
    static constexpr uint32_t kMaxEntities  = 4096u;
    static constexpr uint32_t kMaxComponents =
        static_cast<uint32_t>(ComponentType::MAX_COMPONENTS);

    // Entity storage layout.
    // Low 16 bits = slot index; high 16 bits = generation.
    static constexpr EntityHandle make_handle(uint16_t idx, uint16_t gen) noexcept {
        return (static_cast<uint32_t>(gen) << 16u) | idx;
    }
    static constexpr uint16_t handle_index(EntityHandle h) noexcept {
        return static_cast<uint16_t>(h & 0xFFFFu);
    }
    static constexpr uint16_t handle_generation(EntityHandle h) noexcept {
        return static_cast<uint16_t>(h >> 16u);
    }

    explicit Chunk(const Archetype& archetype, std::size_t component_size);
    ~Chunk() = default;

    Chunk(const Chunk&)            = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&&)                 = default;
    Chunk& operator=(Chunk&&)      = default;

    // --- Entity lifecycle --------------------------------------------------

    // Allocate a slot, assign a handle, zero-initialise component storage.
    // Returns INVALID_ENTITY if the chunk is full.
    [[nodiscard]] EntityHandle spawn_entity();

    // Free the slot.  The generation counter is incremented so existing
    // handles become stale immediately.
    void destroy_entity(EntityHandle entity);

    [[nodiscard]] bool is_alive(EntityHandle entity) const noexcept;
    [[nodiscard]] bool is_full()  const noexcept { return free_count_ == 0u; }
    [[nodiscard]] uint32_t live_count() const noexcept { return live_count_; }

    // --- Component access --------------------------------------------------

    // Return a pointer to the component data for this entity.
    // T::TYPE_ID must map to a ComponentType entry.
    // Returns nullptr if the slot is not alive.
    template<typename T>
    [[nodiscard]] T* get_component(EntityHandle entity) noexcept;

    template<typename T>
    [[nodiscard]] const T* get_component(EntityHandle entity) const noexcept;

    // Write a component value into the slot.
    template<typename T>
    void set_component(EntityHandle entity, const T& value) noexcept;

    // --- Bulk iteration (used by Query) ------------------------------------

    // Raw pointer + count for a given component type.  Useful for SIMD loops.
    [[nodiscard]] void* raw_component_array(ComponentType ct) noexcept;
    [[nodiscard]] const void* raw_component_array(ComponentType ct) const noexcept;

    // Iterator over live entities.
    class Iterator {
    public:
        explicit Iterator(const Chunk& chunk, uint32_t idx = 0u)
            : chunk_(&chunk), idx_(idx) { advance_to_live(); }

        [[nodiscard]] EntityHandle entity() const noexcept {
            return chunk_->make_handle(
                static_cast<uint16_t>(idx_),
                chunk_->generation_[idx_]);
        }
        Iterator& operator++() { ++idx_; advance_to_live(); return *this; }
        bool operator!=(const Iterator& o) const noexcept { return idx_ != o.idx_; }

    private:
        void advance_to_live() {
            while (idx_ < kMaxEntities && !chunk_->alive_[idx_]) ++idx_;
        }
        const Chunk* chunk_;
        uint32_t     idx_;
    };

    [[nodiscard]] Iterator begin() const noexcept { return Iterator{*this, 0u}; }
    [[nodiscard]] Iterator end()   const noexcept { return Iterator{*this, kMaxEntities}; }

private:
    // Per-component SOA storage (flat byte arrays, one per ComponentType slot).
    std::vector<uint8_t>  component_data_[kMaxComponents];
    std::size_t           component_stride_[kMaxComponents]{};

    // Slot bookkeeping.
    bool     alive_[kMaxEntities]{};
    uint16_t generation_[kMaxEntities]{};

    // Free-list as a stack of slot indices.
    uint16_t free_stack_[kMaxEntities]{};
    uint32_t free_count_{kMaxEntities};
    uint32_t live_count_{0u};

    // Cache the archetype mask for validity checks.
    uint32_t archetype_mask_{0u};
};

// ---------------------------------------------------------------------------
// Template implementation
// ---------------------------------------------------------------------------

template<typename T>
T* Chunk::get_component(EntityHandle entity) noexcept {
    if (!is_alive(entity)) return nullptr;
    const uint32_t ct_idx = static_cast<uint32_t>(T::TYPE_ID);
    if (component_stride_[ct_idx] == 0) return nullptr;
    const uint16_t slot = handle_index(entity);
    return reinterpret_cast<T*>(
        component_data_[ct_idx].data() + slot * component_stride_[ct_idx]);
}

template<typename T>
const T* Chunk::get_component(EntityHandle entity) const noexcept {
    if (!is_alive(entity)) return nullptr;
    const uint32_t ct_idx = static_cast<uint32_t>(T::TYPE_ID);
    if (component_stride_[ct_idx] == 0) return nullptr;
    const uint16_t slot = handle_index(entity);
    return reinterpret_cast<const T*>(
        component_data_[ct_idx].data() + slot * component_stride_[ct_idx]);
}

template<typename T>
void Chunk::set_component(EntityHandle entity, const T& value) noexcept {
    T* ptr = get_component<T>(entity);
    if (ptr) *ptr = value;
}

} // namespace ecs
} // namespace gw
