#include "chunk.hpp"
#include "archetype.hpp"
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace gw {
namespace ecs {

Chunk::Chunk(const Archetype& archetype, std::size_t component_size) {
    archetype_mask_ = archetype.component_mask();

    // Initialise free-list as [0, 1, 2, ..., kMaxEntities-1]
    for (uint32_t i = 0u; i < kMaxEntities; ++i) {
        free_stack_[i] = static_cast<uint16_t>(i);
    }
    free_count_ = kMaxEntities;

    // Allocate per-component arrays for every component type present in
    // the archetype.  If the archetype does not include a component type the
    // array stays empty (stride stays 0), acting as a sentinel.
    (void)component_size; // forward-compat param; callers may pass sizeof(T)
    for (uint32_t ct = 0u; ct < kMaxComponents; ++ct) {
        if (archetype_mask_ & (1u << ct)) {
            // Conservative: allocate kMaxEntities × size slots.  A more
            // production-grade impl would pass per-type sizes; here we use
            // the caller-supplied default.  Phase 3 follow-up: per-type
            // registration via a component_registry.
            const std::size_t stride = (component_size > 0) ? component_size : 64u;
            component_stride_[ct] = stride;
            component_data_[ct].resize(kMaxEntities * stride, 0u);
        }
    }
}

EntityHandle Chunk::spawn_entity() {
    if (free_count_ == 0u) return INVALID_ENTITY;
    --free_count_;
    uint16_t slot = free_stack_[free_count_];
    alive_[slot]  = true;
    ++live_count_;

    // Zero-init component data for this slot.
    for (uint32_t ct = 0u; ct < kMaxComponents; ++ct) {
        if (component_stride_[ct] > 0) {
            std::memset(component_data_[ct].data() + slot * component_stride_[ct],
                        0, component_stride_[ct]);
        }
    }

    return make_handle(slot, generation_[slot]);
}

void Chunk::destroy_entity(EntityHandle entity) {
    if (!is_alive(entity)) return;
    const uint16_t slot = handle_index(entity);
    alive_[slot] = false;
    ++generation_[slot]; // invalidate outstanding handles
    free_stack_[free_count_++] = slot;
    --live_count_;
}

bool Chunk::is_alive(EntityHandle entity) const noexcept {
    if (entity == INVALID_ENTITY) return false;
    const uint16_t slot = handle_index(entity);
    const uint16_t gen  = handle_generation(entity);
    if (slot >= kMaxEntities) return false;
    return alive_[slot] && generation_[slot] == gen;
}

void* Chunk::raw_component_array(ComponentType ct) noexcept {
    const uint32_t idx = static_cast<uint32_t>(ct);
    if (idx >= kMaxComponents || component_data_[idx].empty()) return nullptr;
    return component_data_[idx].data();
}

const void* Chunk::raw_component_array(ComponentType ct) const noexcept {
    const uint32_t idx = static_cast<uint32_t>(ct);
    if (idx >= kMaxComponents || component_data_[idx].empty()) return nullptr;
    return component_data_[idx].data();
}

} // namespace ecs
} // namespace gw
