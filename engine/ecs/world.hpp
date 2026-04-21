#pragma once
// engine/ecs/world.hpp
// Top-level ECS container. See docs/adr/0004-ecs-world.md §2.1.

#include "archetype.hpp"
#include "component_registry.hpp"
#include "entity.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace gw {
namespace ecs {

struct EntitySlot {
    std::uint32_t generation   = 0;   // 0 == free
    ArchetypeId   archetype_id = kInvalidArchetypeId;
    std::uint16_t chunk_index  = std::numeric_limits<std::uint16_t>::max();
    std::uint16_t slot_index   = std::numeric_limits<std::uint16_t>::max();
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
    void                 destroy_entity(Entity e);
    [[nodiscard]] bool   is_alive(Entity e) const noexcept;

    [[nodiscard]] std::size_t entity_count() const noexcept { return live_entity_count_; }

    // Stable-order visit over live entities.
    void visit_entities(const std::function<void(Entity)>& fn) const;

    // -----------------------------------------------------------------
    // Component add / remove / access
    // -----------------------------------------------------------------
    template <typename T>
    T& add_component(Entity e, T value = T{});

    template <typename T>
    void remove_component(Entity e);

    template <typename T>
    [[nodiscard]] bool has_component(Entity e) const noexcept;

    template <typename T>
    [[nodiscard]] T* get_component(Entity e) noexcept;

    template <typename T>
    [[nodiscard]] const T* get_component(Entity e) const noexcept;

    // -----------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------
    template <typename... Cs, typename Fn>
    void for_each(Fn&& fn);

    // -----------------------------------------------------------------
    // Hierarchy (§2.7). Implemented on top of HierarchyComponent; all four
    // helpers are no-ops for entities without a HierarchyComponent.
    // -----------------------------------------------------------------

    // Returns true on success; false if `new_parent` is a descendant of
    // `child` (would create a cycle) or if either entity is dead.
    [[nodiscard]] bool reparent(Entity child, Entity new_parent);

    // DFS over the subtree rooted at `root` (including `root` itself).
    void visit_descendants(Entity root, const std::function<void(Entity)>& fn) const;

    // Every entity whose HierarchyComponent.parent.is_null(), i.e. a root.
    void visit_roots(const std::function<void(Entity)>& fn) const;

    // Convenience: collect immediate children into a vector.
    [[nodiscard]] std::vector<Entity> children_of(Entity e) const;

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
    [[nodiscard]] const EntitySlot* slot_for(Entity e) const noexcept;

private:
    // Low-level: migrate `e` to the archetype identified by (new_mask, new_types_sorted).
    // Returns pointer to the newly-allocated component slot for `added_tid` (or
    // nullptr if not an add). Non-template; callable from template wrappers.
    std::byte* migrate_entity_(Entity                        e,
                                 ComponentMask                new_mask,
                                 const std::vector<ComponentTypeId>& new_types_sorted,
                                 ComponentTypeId               added_tid);

    // Raw component pointer for (e, tid) or nullptr if e doesn't have the
    // component or is dead.
    [[nodiscard]] std::byte*       raw_component_(Entity e, ComponentTypeId tid) noexcept;
    [[nodiscard]] const std::byte* raw_component_(Entity e, ComponentTypeId tid) const noexcept;

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
T& World::add_component(Entity e, T value) {
    const auto tid = registry_.id_of<T>();
    if (!is_alive(e)) {
        // Caller bug — accept silently and return a scratch location? Safer
        // to throw so the bug is visible. But we keep the ECS "no throw in
        // steady-state" by requiring callers check is_alive.
        throw std::runtime_error("World::add_component: entity is dead");
    }

    auto&        slot     = slots_[e.index()];
    ComponentMask new_mask;
    std::vector<ComponentTypeId> new_types;
    if (slot.archetype_id != kInvalidArchetypeId) {
        const auto& cur = archetypes_.archetype(slot.archetype_id);
        new_mask        = cur.mask();
        new_types       = cur.types();
    }
    if (new_mask.test(tid)) {
        // Already present — overwrite in place.
        auto* raw = raw_component_(e, tid);
        auto* dst = std::launder(reinterpret_cast<T*>(raw));
        *dst      = std::move(value);
        return *dst;
    }
    new_mask.set(tid);
    new_types.push_back(tid);
    std::sort(new_types.begin(), new_types.end());

    auto* dst_raw = migrate_entity_(e, new_mask, new_types, tid);
    // Construct the new component into `dst_raw` by (move-)assigning from value.
    auto* dst = ::new (static_cast<void*>(dst_raw)) T(std::move(value));
    return *dst;
}

template <typename T>
void World::remove_component(Entity e) {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) return;          // never registered
    if (!is_alive(e)) return;

    auto& slot = slots_[e.index()];
    if (slot.archetype_id == kInvalidArchetypeId) return;

    const auto& cur = archetypes_.archetype(slot.archetype_id);
    if (!cur.mask().test(tid)) return;                    // not present

    ComponentMask new_mask  = cur.mask();
    new_mask.reset(tid);
    std::vector<ComponentTypeId> new_types;
    new_types.reserve(cur.types().size() - 1);
    for (auto t : cur.types()) {
        if (t != tid) new_types.push_back(t);
    }

    migrate_entity_(e, new_mask, new_types, kInvalidComponentTypeId);
}

template <typename T>
bool World::has_component(Entity e) const noexcept {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) return false;
    if (!is_alive(e)) return false;
    const auto& slot = slots_[e.index()];
    if (slot.archetype_id == kInvalidArchetypeId) return false;
    return archetypes_.archetype(slot.archetype_id).mask().test(tid);
}

template <typename T>
T* World::get_component(Entity e) noexcept {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) return nullptr;
    auto* raw = raw_component_(e, tid);
    return raw ? std::launder(reinterpret_cast<T*>(raw)) : nullptr;
}

template <typename T>
const T* World::get_component(Entity e) const noexcept {
    const auto tid = registry_.id_of_or_invalid<T>();
    if (tid == kInvalidComponentTypeId) return nullptr;
    const auto* raw = raw_component_(e, tid);
    return raw ? std::launder(reinterpret_cast<const T*>(raw)) : nullptr;
}

template <typename... Cs, typename Fn>
void World::for_each(Fn&& fn) {
    static_assert(sizeof...(Cs) >= 1, "for_each<> requires at least one component type");

    ComponentTypeId ids[] = { registry_.id_of<Cs>()... };
    ComponentMask required;
    for (auto id : ids) required.set(id);

    archetypes_.for_each_matching(required, [&](ArchetypeId aid) {
        auto& arch = archetypes_.archetype(aid);
        // Precompute per-component byte offsets for this archetype, in the
        // order the caller's Fn expects them.
        std::size_t comp_offsets[sizeof...(Cs)];
        std::size_t comp_sizes[sizeof...(Cs)];
        for (std::size_t i = 0; i < sizeof...(Cs); ++i) {
            comp_offsets[i] = arch.offset_of(ids[i]);
            comp_sizes[i]   = registry_.info(ids[i]).size;
        }

        for (std::size_t ci = 0; ci < arch.chunk_count(); ++ci) {
            auto& chunk = arch.chunk(ci);
            const auto live = chunk.live_count();
            for (std::uint16_t s = 0; s < live; ++s) {
                [&]<std::size_t... I>(std::index_sequence<I...>) {
                    fn(chunk.entity_at(s),
                       *std::launder(reinterpret_cast<Cs*>(
                           chunk.component_ptr(comp_offsets[I], comp_sizes[I], s)))...);
                }(std::make_index_sequence<sizeof...(Cs)>{});
            }
        }
    });
}

} // namespace ecs
} // namespace gw
