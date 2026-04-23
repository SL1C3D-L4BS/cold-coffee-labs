#pragma once
// engine/ecs/world.hpp
// Top-level ECS container. See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.1.
//
// IWYU audit (audit-2026-04-20 P5): every include below is load-bearing for the
// inline template machinery at the foot of this header. We cannot drop any
// without breaking a consumer. The god-header pressure this header exerts
// ~350 KLOC across the engine is the cost of exposing archetype/chunk
// internals so that `for_each<>`, `add_component<>`, `get_component<>` can
// inline into the hot loop.
//
// Future-work burn-down (tracked in docs/10 ADR-0094 skeleton, not yet filed):
//   1. Move the template implementations (§ "Template implementations" below)
//      into `world.inl` included only at the bottom, so every consumer of
//      `World` still pays the same include cost today but a future
//      `IWorld` forward-decl can be split out into `world_fwd.hpp` for
//      headers that only need to refer to `World*`.
//   2. Most subsystems pass `World&` through and only need a forward-decl;
//      `engine/ecs/world_fwd.hpp` (not yet extracted) would break the
//      header chain.  Estimated .cpp recompile win: 30-40 % for
//      engine/input, engine/telemetry, engine/persist headers that only
//      need `class World;`.
// The ratchet approach: when a consumer is refactored to take `World*`
// through a pImpl or interface, flip its `#include "engine/ecs/world.hpp"` to
// `#include "engine/ecs/world_fwd.hpp"`. No one-shot sweep.

#include "archetype.hpp"            // ArchetypeId, ArchetypeTable, kInvalidArchetypeId, archetype() — full type required for inline template access.
#include "component_registry.hpp"   // ComponentRegistry + ComponentTypeId / ComponentMask — required by template bodies.
#include "entity.hpp"               // Entity, Entity::null(), index() — trivially-copyable handle type, inlined everywhere.

#include <algorithm>    // std::ranges::sort in add_component<> template body.
#include <array>        // std::array in for_each<> template body.
#include <cstddef>      // std::byte (migrate_entity_ return type) + std::size_t.
#include <cstdint>      // std::uint16_t / std::uint32_t / std::uint64_t in EntitySlot + restore_entity_with_bits.
#include <functional>   // std::function<void(Entity)> used by visit_*.
#include <limits>       // std::numeric_limits<std::uint16_t>::max() sentinel in EntitySlot.
#include <new>          // std::launder + placement-new in add_component<>.
#include <stdexcept>    // std::runtime_error thrown once in add_component<>; quarantined exception boundary per ADR-0086.
#include <utility>      // std::move / std::forward / std::index_sequence used by for_each<>.
#include <vector>       // std::vector return in children_of + internal slot table.

namespace gw::ecs {

struct EntitySlot {
    // Monotonically-increasing version counter. Bumped on every create AND on
    // every destroy so that a handle captured before destroy never passes the
    // is_alive check again, even after the slot is reused. Never 0 after first
    // use — the create path skips 0 to preserve Entity::null() as "never seen".
    std::uint32_t generation   = 0;
    ArchetypeId   archetype_id = kInvalidArchetypeId;
    std::uint16_t chunk_index  = std::numeric_limits<std::uint16_t>::max();
    std::uint16_t slot_index   = std::numeric_limits<std::uint16_t>::max();
    // True iff this slot currently represents a live entity. Distinct from
    // generation so destroy_entity can bump generation (defeating stale
    // handles) without re-using 0 as "free" — see ADR-0004 §2.5.
    bool          alive        = false;
};

class World {
public:
    World();
    ~World();

    World(const World&)            = delete;
    World& operator=(const World&) = delete;
    World(World&&) noexcept;
    World& operator=(World&&) noexcept;

    // -----------------------------------------------------------------
    // Entity lifecycle
    // -----------------------------------------------------------------
    [[nodiscard]] Entity create_entity();
    void                 destroy_entity(Entity entity);
    [[nodiscard]] bool   is_alive(Entity entity) const noexcept;

    [[nodiscard]] std::size_t entity_count() const noexcept { return live_entity_count_; }

    // Stable-order visit over live entities.
    void visit_entities(const std::function<void(Entity)>& visitor) const;

    // -----------------------------------------------------------------
    // Component add / remove / access
    // -----------------------------------------------------------------
    template <typename T>
    T& add_component(Entity entity, T value = T{});

    template <typename T>
    void remove_component(Entity entity);

    template <typename T>
    [[nodiscard]] bool has_component(Entity entity) const noexcept;

    template <typename T>
    [[nodiscard]] T* get_component(Entity entity) noexcept;

    template <typename T>
    [[nodiscard]] const T* get_component(Entity entity) const noexcept;

    // -----------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------
    template <typename... Cs, typename Fn>
    void for_each(Fn&& visitor);

    // -----------------------------------------------------------------
    // Hierarchy (§2.7). Implemented on top of HierarchyComponent; all four
    // helpers are no-ops for entities without a HierarchyComponent.
    // -----------------------------------------------------------------

    // Returns true on success; false if `new_parent` is a descendant of
    // `child` (would create a cycle) or if either entity is dead.
    [[nodiscard]] bool reparent(Entity child, Entity new_parent);

    // DFS over the subtree rooted at `root` (including `root` itself).
    void visit_descendants(Entity root, const std::function<void(Entity)>& visitor) const;

    // Every entity whose HierarchyComponent.parent.is_null(), i.e. a root.
    void visit_roots(const std::function<void(Entity)>& visitor) const;

    // Convenience: collect immediate children into a vector.
    [[nodiscard]] std::vector<Entity> children_of(Entity entity) const;

    // -----------------------------------------------------------------
    // Registries (for reflection / serialization / inspector)
    // -----------------------------------------------------------------
    [[nodiscard]] ComponentRegistry&       component_registry()       noexcept { return registry_; }
    [[nodiscard]] const ComponentRegistry& component_registry() const noexcept { return registry_; }
    [[nodiscard]] ArchetypeTable&          archetype_table()          noexcept { return archetypes_; }
    [[nodiscard]] const ArchetypeTable&    archetype_table()    const noexcept { return archetypes_; }

    // -----------------------------------------------------------------
    // Raw entity-slot access (used by serialization + debug inspector).
    // -----------------------------------------------------------------
    [[nodiscard]] const EntitySlot* slot_for(Entity entity) const noexcept;

    // -----------------------------------------------------------------
    // Serialization helpers (ADR-0006). The ComponentRegistry is preserved
    // across clear() — the registered types outlive the entity data.
    // -----------------------------------------------------------------

    // Destroy every live entity, empty every archetype chunk, and reset the
    // entity table. ComponentRegistry is left untouched.
    void clear() noexcept;

    // Create an entity whose bits exactly match `bits`. Used by load_world to
    // preserve intra-blob Entity references (Hierarchy components). Returns
    // Entity::null() when the request conflicts with a live slot or when the
    // generation in `bits` is 0 (reserved).
    Entity restore_entity_with_bits(std::uint64_t bits);

    // Add a trivially-copyable component by ComponentTypeId using raw bytes.
    // Returns true on success; false if `e` is dead, `tid` is invalid, or the
    // component is non-trivially-copyable. Size must match registry info.
    bool add_component_raw(Entity entity, ComponentTypeId tid,
                            const void* src, std::size_t size);

private:
    // Low-level: migrate `entity` to the archetype identified by (new_mask, new_types_sorted).
    // Returns pointer to the newly-allocated component slot for `added_tid` (or
    // nullptr if not an add). Non-template; callable from template wrappers.
    std::byte* migrate_entity_(Entity                        entity,
                                 ComponentMask                new_mask,
                                 const std::vector<ComponentTypeId>& new_types_sorted,
                                 ComponentTypeId               added_tid);

    // Raw component pointer for (entity, tid) or nullptr if entity doesn't have the
    // component or is dead.
    [[nodiscard]] std::byte*       raw_component_(Entity entity, ComponentTypeId tid) noexcept;
    [[nodiscard]] const std::byte* raw_component_(Entity entity, ComponentTypeId tid) const noexcept;

    ComponentRegistry              registry_;
    ArchetypeTable                 archetypes_;

    // EntityTable: index → slot. Grows indefinitely; freed indices are
    // re-used via `free_list_`.
    std::vector<EntitySlot>        slots_;
    std::vector<std::uint32_t>     free_list_;
    std::size_t                    live_entity_count_ = 0;
};

// ---------------------------------------------------------------------------
// Template implementations
// ---------------------------------------------------------------------------

template <typename T>
T& World::add_component(Entity entity, T value) {
    const auto tid = registry_.id_of<T>();
    if (!is_alive(entity)) {
        // Caller bug — accept silently and return a scratch location? Safer
        // to throw so the bug is visible. But we keep the ECS "no throw in
        // steady-state" by requiring callers check is_alive.
        throw std::runtime_error("World::add_component: entity is dead");
    }

    auto&        slot     = slots_[entity.index()];
    ComponentMask new_mask;
    std::vector<ComponentTypeId> new_types;
    if (slot.archetype_id != kInvalidArchetypeId) {
        const auto& cur = archetypes_.archetype(slot.archetype_id);
        new_mask        = cur.mask();
        new_types       = cur.types();
    }
    if (new_mask.test(tid)) {
        // Already present — overwrite in place.
        auto* raw = raw_component_(entity, tid);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — typed view of raw storage
        auto* dst = std::launder(reinterpret_cast<T*>(raw));
        *dst      = std::move(value);
        return *dst;
    }
    new_mask.set(tid);
    new_types.push_back(tid);
    std::ranges::sort(new_types);

    auto* dst_raw = migrate_entity_(entity, new_mask, new_types, tid);
    // Construct the new component into `dst_raw` by (move-)assigning from value.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) — placement-new into archetype slab; World owns lifetime
    auto* dst = ::new (static_cast<void*>(dst_raw)) T(std::move(value));
    return *dst;
}

template <typename T>
void World::remove_component(Entity entity) {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) {
        return;   // never registered
    }
    if (!is_alive(entity)) {
        return;
    }

    auto& slot = slots_[entity.index()];
    if (slot.archetype_id == kInvalidArchetypeId) {
        return;
    }

    const auto& cur = archetypes_.archetype(slot.archetype_id);
    if (!cur.mask().test(tid)) {
        return;   // not present
    }

    ComponentMask new_mask  = cur.mask();
    new_mask.reset(tid);
    std::vector<ComponentTypeId> new_types;
    new_types.reserve(cur.types().size() - 1);
    for (auto type_id : cur.types()) {
        if (type_id != tid) {
            new_types.push_back(type_id);
        }
    }

    migrate_entity_(entity, new_mask, new_types, kInvalidComponentTypeId);
}

template <typename T>
bool World::has_component(Entity entity) const noexcept {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) {
        return false;
    }
    if (!is_alive(entity)) {
        return false;
    }
    const auto& slot = slots_[entity.index()];
    if (slot.archetype_id == kInvalidArchetypeId) {
        return false;
    }
    return archetypes_.archetype(slot.archetype_id).mask().test(tid);
}

template <typename T>
T* World::get_component(Entity entity) noexcept {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) {
        return nullptr;
    }
    auto* raw = raw_component_(entity, tid);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return (raw != nullptr) ? std::launder(reinterpret_cast<T*>(raw)) : nullptr;
}

template <typename T>
const T* World::get_component(Entity entity) const noexcept {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) {
        return nullptr;
    }
    const auto* raw = raw_component_(entity, tid);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return (raw != nullptr) ? std::launder(reinterpret_cast<const T*>(raw)) : nullptr;
}

template <typename... Cs, typename Fn>
void World::for_each(Fn&& visitor) {
    static_assert(sizeof...(Cs) >= 1, "for_each<> requires at least one component type");

    const std::array<ComponentTypeId, sizeof...(Cs)> ids{registry_.id_of<Cs>()...};
    ComponentMask required;
    for (auto comp_type_id : ids) {
        required.set(comp_type_id);
    }

    archetypes_.for_each_matching(required, [this, visitor = std::forward<Fn>(visitor), ids](ArchetypeId aid) {
        auto& arch = archetypes_.archetype(aid);
        // Precompute per-component byte offsets for this archetype, in the
        // order the caller's visitor expects them.
        std::array<std::size_t, sizeof...(Cs)> comp_offsets{};
        std::array<std::size_t, sizeof...(Cs)> comp_sizes{};
        for (std::size_t ix = 0; ix < sizeof...(Cs); ++ix) {
            comp_offsets[ix] = arch.offset_of(ids[ix]);
            comp_sizes[ix]   = registry_.info(ids[ix]).size;
        }

        for (std::size_t chunk_ix = 0; chunk_ix < arch.chunk_count(); ++chunk_ix) {
            auto& chunk = arch.chunk(chunk_ix);
            const auto live = chunk.live_count();
            for (std::uint16_t slot_ix = 0; slot_ix < live; ++slot_ix) {
                [&]<std::size_t... I>(std::index_sequence<I...>) {
                    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
                    visitor(
                        chunk.entity_at(slot_ix),
                        *std::launder(reinterpret_cast<Cs*>(
                            chunk.component_ptr(comp_offsets[I], comp_sizes[I], slot_ix)))...);
                    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
                }(std::make_index_sequence<sizeof...(Cs)>{});
            }
        }
    });
}

} // namespace gw::ecs
