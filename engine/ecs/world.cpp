// engine/ecs/world.cpp
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md.

#include "world.hpp"

#include "hierarchy.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace gw::ecs {

World::World() {
    slots_.reserve(256);
    free_list_.reserve(64);
}

World::~World() {
    // Walk every live entity in every archetype and run destructors on the
    // non-trivially-destructible components. Required because chunks own
    // raw byte storage and don't know their contained types.
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        auto& arch = archetypes_.archetype(aid);
        for (std::size_t chunk_ix = 0; chunk_ix < arch.chunk_count(); ++chunk_ix) {
            auto& chunk = arch.chunk(chunk_ix);
            const auto live = chunk.live_count();
            for (std::uint16_t slot_ix = 0; slot_ix < live; ++slot_ix) {
                arch.destruct_components_at(registry_, static_cast<std::uint16_t>(chunk_ix),
                                            slot_ix);
            }
        }
    }
}

World::World(World&&) noexcept            = default;
World& World::operator=(World&&) noexcept = default;

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

Entity World::create_entity() {
    std::uint32_t index = 0;
    if (!free_list_.empty()) {
        index = free_list_.back();
        free_list_.pop_back();
    } else {
        if (slots_.size() == std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("World::create_entity: index space exhausted (2^32)");
        }
        index = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();   // default-constructed: generation=0, archetype invalid
    }
    auto& slot = slots_[index];
    // Bump generation on every create. The destroy path also bumps, so a
    // create → destroy → create sequence on the same slot advances the
    // generation twice, guaranteeing strict > ordering (ADR-0004 §2.3
    // generation-bump rule).
    ++slot.generation;
    if (slot.generation == 0) {
        slot.generation = 1;   // skip the null sentinel
    }
    slot.alive        = true;
    slot.archetype_id = kInvalidArchetypeId;
    slot.chunk_index  = std::numeric_limits<std::uint16_t>::max();
    slot.slot_index   = std::numeric_limits<std::uint16_t>::max();
    ++live_entity_count_;
    return Entity::from_parts(index, slot.generation);
}

void World::destroy_entity(Entity entity) {
    if (!is_alive(entity)) {
        return;
    }
    auto& slot = slots_[entity.index()];

    if (slot.archetype_id != kInvalidArchetypeId) {
        auto& arch          = archetypes_.archetype(slot.archetype_id);
        const auto chunk_ix = slot.chunk_index;
        const auto slot_ix  = slot.slot_index;
        arch.destruct_components_at(registry_, chunk_ix, slot_ix);
        const Entity swapped = arch.free_slot_swap_back(registry_, chunk_ix, slot_ix);
        if (!swapped.is_null()) {
            // The entity that moved into (chunk_ix, slot_ix) now points to the vacated slot.
            auto& swapped_slot       = slots_[swapped.index()];
            swapped_slot.chunk_index = chunk_ix;
            swapped_slot.slot_index  = slot_ix;
        }
    }

    // Mark the slot free AND bump the generation so any stale copy of the
    // outgoing handle (same index + pre-destroy generation) is rejected
    // immediately by is_alive, and so a subsequent create_entity() on this
    // same slot lands on a strictly-greater generation than what the caller
    // is still holding. ADR-0004 §2.3 generation-bump rule.
    slot.alive        = false;
    ++slot.generation;
    if (slot.generation == 0) {
        slot.generation = 1;
    }
    slot.archetype_id = kInvalidArchetypeId;
    slot.chunk_index  = std::numeric_limits<std::uint16_t>::max();
    slot.slot_index   = std::numeric_limits<std::uint16_t>::max();
    free_list_.push_back(entity.index());
    --live_entity_count_;
}

bool World::is_alive(Entity entity) const noexcept {
    if (entity.is_null()) {
        return false;
    }
    if (entity.index() >= slots_.size()) {
        return false;
    }
    const auto& slot = slots_[entity.index()];
    return slot.alive && slot.generation == entity.generation();
}

void World::visit_entities(const std::function<void(Entity)>& visitor) const {
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        const auto& arch = archetypes_.archetype(aid);
        for (std::size_t chunk_ix = 0; chunk_ix < arch.chunk_count(); ++chunk_ix) {
            const auto& chunk = arch.chunk(chunk_ix);
            const auto live = chunk.live_count();
            for (std::uint16_t slot_ix = 0; slot_ix < live; ++slot_ix) {
                visitor(chunk.entity_at(slot_ix));
            }
        }
    }
    // Entities with no components (archetype_id == invalid) are live too —
    // emit them so visit_entities matches entity_count(). Freed slots carry
    // alive == false and are skipped.
    for (std::uint32_t slot_index = 0; slot_index < slots_.size(); ++slot_index) {
        const auto& slot = slots_[slot_index];
        if (slot.alive && slot.archetype_id == kInvalidArchetypeId) {
            visitor(Entity::from_parts(slot_index, slot.generation));
        }
    }
}

const EntitySlot* World::slot_for(Entity entity) const noexcept {
    if (!is_alive(entity)) {
        return nullptr;
    }
    return &slots_[entity.index()];
}

// ---------------------------------------------------------------------------
// Serialization helpers (ADR-0006 §2.9)
// ---------------------------------------------------------------------------

void World::clear() noexcept {
    // Walk every live entity in every archetype and run destructors on the
    // non-trivially-destructible components (mirrors ~World); then reset.
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        auto& arch = archetypes_.archetype(aid);
        for (std::size_t chunk_ix = 0; chunk_ix < arch.chunk_count(); ++chunk_ix) {
            auto& chunk = arch.chunk(chunk_ix);
            const auto live = chunk.live_count();
            for (std::uint16_t slot_ix = 0; slot_ix < live; ++slot_ix) {
                arch.destruct_components_at(registry_, static_cast<std::uint16_t>(chunk_ix),
                                            slot_ix);
            }
        }
    }
    // Replace the archetype table wholesale — the registry is preserved so
    // subsequent id_of<T>() lookups stay stable.
    archetypes_ = ArchetypeTable{};
    slots_.clear();
    free_list_.clear();
    live_entity_count_ = 0;
}

Entity World::restore_entity_with_bits(std::uint64_t bits) {
    const Entity entity = Entity::from_raw_bits(bits);
    if (entity.generation() == 0) {
        return Entity::null();
    }
    const auto idx = entity.index();
    // Grow the slot table if needed; freshly created slots have gen=0.
    if (idx >= slots_.size()) {
        slots_.resize(static_cast<std::size_t>(idx) + 1);
    }
    auto& slot = slots_[idx];
    // Must point at an empty slot — anything else would collide with a live
    // entity and is a caller bug. For deserialization, `clear()` is the
    // contractual predecessor, so every slot we walk here is born with
    // alive=false.
    if (slot.alive) {
        return Entity::null();
    }

    slot.generation   = entity.generation();
    slot.alive        = true;
    slot.archetype_id = kInvalidArchetypeId;
    slot.chunk_index  = std::numeric_limits<std::uint16_t>::max();
    slot.slot_index   = std::numeric_limits<std::uint16_t>::max();
    ++live_entity_count_;

    // Remove this index from the free list if it happens to be there.
    const auto pos = std::ranges::find(free_list_, idx);
    if (pos != free_list_.end()) {
        free_list_.erase(pos);
    }

    return entity;
}

bool World::add_component_raw(Entity entity, ComponentTypeId tid, const void* src,
                              std::size_t size) {
    if (!is_alive(entity)) {
        return false;
    }
    if (tid == kInvalidComponentTypeId) {
        return false;
    }
    if (tid >= registry_.component_count()) {
        return false;
    }

    const auto& info = registry_.info(tid);
    if (size != info.size) {
        return false;
    }
    // Non-trivially-copyable types must go through add_component<T>; the
    // serialization reflection path lands with ADR-0006 §2.5.
    if (!info.trivially_copyable) {
        return false;
    }

    auto& slot = slots_[entity.index()];
    ComponentMask new_mask;
    std::vector<ComponentTypeId> new_types;
    if (slot.archetype_id != kInvalidArchetypeId) {
        const auto& cur = archetypes_.archetype(slot.archetype_id);
        new_mask        = cur.mask();
        new_types       = cur.types();
    }
    if (new_mask.test(tid)) {
        auto* dst = raw_component_(entity, tid);
        if (dst != nullptr && src != nullptr) {
            std::memcpy(dst, src, size);
        }
        return true;
    }
    new_mask.set(tid);
    new_types.push_back(tid);
    std::ranges::sort(new_types);

    auto* dst_raw = migrate_entity_(entity, new_mask, new_types, tid);
    if (dst_raw == nullptr || src == nullptr) {
        return false;
    }
    std::memcpy(dst_raw, src, size);
    return true;
}

// ---------------------------------------------------------------------------
// Raw component access
// ---------------------------------------------------------------------------

std::byte* World::raw_component_(Entity entity, ComponentTypeId tid) noexcept {
    if (!is_alive(entity)) {
        return nullptr;
    }
    const auto& slot = slots_[entity.index()];
    if (slot.archetype_id == kInvalidArchetypeId) {
        return nullptr;
    }
    auto& arch = archetypes_.archetype(slot.archetype_id);
    return arch.component_ptr(registry_, tid, slot.chunk_index, slot.slot_index);
}

const std::byte* World::raw_component_(Entity entity, ComponentTypeId tid) const noexcept {
    if (!is_alive(entity)) {
        return nullptr;
    }
    const auto& slot = slots_[entity.index()];
    if (slot.archetype_id == kInvalidArchetypeId) {
        return nullptr;
    }
    const auto& arch = archetypes_.archetype(slot.archetype_id);
    return arch.component_ptr(registry_, tid, slot.chunk_index, slot.slot_index);
}

// ---------------------------------------------------------------------------
// Entity migration
// ---------------------------------------------------------------------------

// NOLINTBEGIN(readability-function-cognitive-complexity)
std::byte* World::migrate_entity_(Entity entity, ComponentMask new_mask,
                                  const std::vector<ComponentTypeId>& new_types_sorted,
                                  ComponentTypeId added_tid) {
    auto& slot = slots_[entity.index()];

    // Look up / create destination archetype.
    const auto dst_aid = archetypes_.get_or_create(new_mask, new_types_sorted, registry_);
    auto&      dst_arch = archetypes_.archetype(dst_aid);
    const auto dst_slot = dst_arch.allocate_slot();

    // Write the entity handle into the new slot.
    dst_arch.chunk(dst_slot.chunk_index).entity_at(dst_slot.slot) = entity;

    // Move every *shared* component from source to destination.
    if (slot.archetype_id != kInvalidArchetypeId) {
        auto& src_arch = archetypes_.archetype(slot.archetype_id);
        const auto src_chunk_ix = slot.chunk_index;
        const auto src_slot_ix  = slot.slot_index;

        for (auto tid : src_arch.types()) {
            if (!new_mask.test(tid)) {
                continue;   // being removed
            }
            const auto& info = registry_.info(tid);
            auto*       src  = src_arch.component_ptr(registry_, tid, src_chunk_ix, src_slot_ix);
            auto*       dst =
                dst_arch.component_ptr(registry_, tid, dst_slot.chunk_index, dst_slot.slot);
            if (info.trivially_copyable) {
                std::memcpy(dst, src, info.size);
            } else {
                info.move_construct(dst, src);
                if (info.destruct != nullptr) {
                    info.destruct(src);
                }
            }
        }

        // Destruct any components that are being *removed* (in src but not
        // in new_mask).
        for (auto tid : src_arch.types()) {
            if (new_mask.test(tid)) {
                continue;
            }
            const auto& info = registry_.info(tid);
            if (info.trivially_copyable) {
                continue;
            }
            auto* src = src_arch.component_ptr(registry_, tid, src_chunk_ix, src_slot_ix);
            if (info.destruct != nullptr) {
                info.destruct(src);
            }
        }

        // Compact source chunk (swap-back).
        const Entity swapped = src_arch.free_slot_swap_back(registry_, src_chunk_ix, src_slot_ix);
        if (!swapped.is_null()) {
            auto& swapped_slot       = slots_[swapped.index()];
            swapped_slot.chunk_index = src_chunk_ix;
            swapped_slot.slot_index  = src_slot_ix;
        }
    }

    // Update entity table row.
    slot.archetype_id = dst_aid;
    slot.chunk_index  = dst_slot.chunk_index;
    slot.slot_index   = dst_slot.slot;

    // If adding, return a pointer to the uninitialized destination for the
    // caller to placement-construct into.
    if (added_tid != kInvalidComponentTypeId) {
        return dst_arch.component_ptr(registry_, added_tid, dst_slot.chunk_index, dst_slot.slot);
    }
    return nullptr;
}
// NOLINTEND(readability-function-cognitive-complexity)

// ---------------------------------------------------------------------------
// Hierarchy
// ---------------------------------------------------------------------------

namespace {

// Returns true if `candidate` is `root` itself or one of its descendants.
// NOLINTBEGIN(misc-no-recursion) — depth-first on editor-sized hierarchies; bounded by graph depth.
bool is_descendant_or_self(const World& world, Entity root, Entity candidate) {
    if (root == candidate) {
        return true;
    }
    const auto* root_hier = world.get_component<HierarchyComponent>(root);
    if (root_hier == nullptr) {
        return false;
    }
    bool found = false;
    Entity child = root_hier->first_child;
    while (!child.is_null() && !found) {
        if (is_descendant_or_self(world, child, candidate)) {
            found = true;
            break;
        }
        const auto* child_hier = world.get_component<HierarchyComponent>(child);
        if (child_hier == nullptr) {
            break;
        }
        child = child_hier->next_sibling;
    }
    return found;
}
// NOLINTEND(misc-no-recursion)

// Unlink `child` from its current parent's child list, if any. Callers must
// ensure `child` has a HierarchyComponent.
void unlink_from_parent(World& world, Entity child) {
    auto* child_hier = world.get_component<HierarchyComponent>(child);
    if (child_hier == nullptr || child_hier->parent.is_null()) {
        return;
    }

    auto* parent_hier = world.get_component<HierarchyComponent>(child_hier->parent);
    if (parent_hier == nullptr) {
        child_hier->parent = Entity::null();
        return;
    }
    if (parent_hier->first_child == child) {
        parent_hier->first_child = child_hier->next_sibling;
    } else {
        Entity walker = parent_hier->first_child;
        while (!walker.is_null()) {
            auto* walker_hier = world.get_component<HierarchyComponent>(walker);
            if (walker_hier == nullptr) {
                break;
            }
            if (walker_hier->next_sibling == child) {
                walker_hier->next_sibling = child_hier->next_sibling;
                break;
            }
            walker = walker_hier->next_sibling;
        }
    }
    child_hier->parent       = Entity::null();
    child_hier->next_sibling = Entity::null();
}

} // namespace

bool World::reparent(Entity child, Entity new_parent) {
    if (!is_alive(child)) {
        return false;
    }
    if (!new_parent.is_null() && !is_alive(new_parent)) {
        return false;
    }
    if (child == new_parent) {
        return false;
    }

    // Ensure child has a HierarchyComponent.
    auto* child_hier = get_component<HierarchyComponent>(child);
    if (child_hier == nullptr) {
        add_component<HierarchyComponent>(child, HierarchyComponent{});
        child_hier = get_component<HierarchyComponent>(child);
    }
    // Cycle check: new_parent must NOT be in the subtree rooted at child.
    if (!new_parent.is_null() && is_descendant_or_self(*this, child, new_parent)) {
        return false;
    }

    unlink_from_parent(*this, child);

    if (!new_parent.is_null()) {
        auto* parent_hier = get_component<HierarchyComponent>(new_parent);
        if (parent_hier == nullptr) {
            add_component<HierarchyComponent>(new_parent, HierarchyComponent{});
            parent_hier = get_component<HierarchyComponent>(new_parent);
        }
        // Re-fetch child's hierarchy — add_component on new_parent may have
        // migrated the archetype table, which keeps `child`'s slot pointer
        // stable (different archetype) but don't assume `child_hier` is still valid
        // across arbitrary migrations; re-fetch to be safe.
        child_hier = get_component<HierarchyComponent>(child);
        child_hier->parent       = new_parent;
        child_hier->next_sibling = parent_hier->first_child;
        parent_hier->first_child = child;
    }
    return true;
}

// NOLINTBEGIN(misc-no-recursion) — preorder DFS over sibling lists; bounded by hierarchy depth.
void World::visit_descendants(Entity root, const std::function<void(Entity)>& visitor) const {
    if (!is_alive(root)) {
        return;
    }
    visitor(root);
    const auto* root_hier = get_component<HierarchyComponent>(root);
    if (root_hier == nullptr) {
        return;
    }

    Entity current = root_hier->first_child;
    while (!current.is_null()) {
        visit_descendants(current, visitor);
        const auto* current_hier = get_component<HierarchyComponent>(current);
        if (current_hier == nullptr) {
            break;
        }
        current = current_hier->next_sibling;
    }
}
// NOLINTEND(misc-no-recursion)

void World::visit_roots(const std::function<void(Entity)>& visitor) const {
    // Every entity with a HierarchyComponent whose parent is null is a root.
    // Entities WITHOUT a HierarchyComponent are also implicit roots (they
    // participate in the scene as loose, parent-less entities).
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        const auto& arch = archetypes_.archetype(aid);
        const bool has_hierarchy = arch.mask().test(
            registry_.id_of_or_invalid<HierarchyComponent>());
        for (std::size_t chunk_ix = 0; chunk_ix < arch.chunk_count(); ++chunk_ix) {
            const auto& chunk = arch.chunk(chunk_ix);
            const auto live = chunk.live_count();
            for (std::uint16_t slot_ix = 0; slot_ix < live; ++slot_ix) {
                const Entity entity = chunk.entity_at(slot_ix);
                if (has_hierarchy) {
                    const auto* hier = get_component<HierarchyComponent>(entity);
                    if (hier != nullptr && !hier->parent.is_null()) {
                        continue;
                    }
                }
                visitor(entity);
            }
        }
    }
    // Archetype-less entities (no components) are also roots.
    for (std::uint32_t slot_index = 0; slot_index < slots_.size(); ++slot_index) {
        const auto& slot = slots_[slot_index];
        if (slot.alive && slot.archetype_id == kInvalidArchetypeId) {
            visitor(Entity::from_parts(slot_index, slot.generation));
        }
    }
}

std::vector<Entity> World::children_of(Entity entity) const {
    std::vector<Entity> out;
    const auto* hier = get_component<HierarchyComponent>(entity);
    if (hier == nullptr) {
        return out;
    }
    Entity current = hier->first_child;
    while (!current.is_null()) {
        out.push_back(current);
        const auto* child_hier = get_component<HierarchyComponent>(current);
        if (child_hier == nullptr) {
            break;
        }
        current = child_hier->next_sibling;
    }
    return out;
}

} // namespace gw::ecs
