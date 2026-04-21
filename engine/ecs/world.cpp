// engine/ecs/world.cpp
// See docs/adr/0004-ecs-world.md.

#include "world.hpp"

#include "hierarchy.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace gw {
namespace ecs {

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
        for (std::size_t ci = 0; ci < arch.chunk_count(); ++ci) {
            auto& chunk = arch.chunk(ci);
            const auto live = chunk.live_count();
            for (std::uint16_t s = 0; s < live; ++s) {
                arch.destruct_components_at(registry_, static_cast<std::uint16_t>(ci), s);
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
    std::uint32_t index;
    if (!free_list_.empty()) {
        index = free_list_.back();
        free_list_.pop_back();
    } else {
        if (slots_.size() == std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("World::create_entity: index space exhausted (2^32)");
        }
        index = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();  // default-constructed: generation=0, archetype invalid
    }
    auto& slot = slots_[index];
    // Bump generation on reuse; new slots transition 0 -> 1.
    ++slot.generation;
    if (slot.generation == 0) slot.generation = 1;  // never use generation 0 for live
    slot.archetype_id = kInvalidArchetypeId;
    slot.chunk_index  = std::numeric_limits<std::uint16_t>::max();
    slot.slot_index   = std::numeric_limits<std::uint16_t>::max();
    ++live_entity_count_;
    return Entity::from_parts(index, slot.generation);
}

void World::destroy_entity(Entity e) {
    if (!is_alive(e)) return;
    auto& slot = slots_[e.index()];

    if (slot.archetype_id != kInvalidArchetypeId) {
        auto& arch       = archetypes_.archetype(slot.archetype_id);
        const auto ci    = slot.chunk_index;
        const auto s     = slot.slot_index;
        arch.destruct_components_at(registry_, ci, s);
        const Entity swapped = arch.free_slot_swap_back(registry_, ci, s);
        if (!swapped.is_null()) {
            // The entity that moved into (ci, s) now points to the vacated slot.
            auto& swapped_slot       = slots_[swapped.index()];
            swapped_slot.chunk_index = ci;
            swapped_slot.slot_index  = s;
        }
    }

    // Mark the slot free. Generation 0 means "free" per EntitySlot's doctrine;
    // a subsequent create_entity() will bump generation back into the live
    // range (1+), so the old handle's generation compare fails is_alive.
    slot.generation   = 0;
    slot.archetype_id = kInvalidArchetypeId;
    slot.chunk_index  = std::numeric_limits<std::uint16_t>::max();
    slot.slot_index   = std::numeric_limits<std::uint16_t>::max();
    free_list_.push_back(e.index());
    --live_entity_count_;
}

bool World::is_alive(Entity e) const noexcept {
    if (e.is_null())                    return false;
    if (e.index() >= slots_.size())     return false;
    const auto& slot = slots_[e.index()];
    return slot.generation == e.generation();
}

void World::visit_entities(const std::function<void(Entity)>& fn) const {
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        const auto& arch = archetypes_.archetype(aid);
        for (std::size_t ci = 0; ci < arch.chunk_count(); ++ci) {
            const auto& chunk = arch.chunk(ci);
            const auto live = chunk.live_count();
            for (std::uint16_t s = 0; s < live; ++s) {
                fn(chunk.entity_at(s));
            }
        }
    }
    // Entities with no components (archetype_id == invalid) are live too —
    // emit them so visit_entities matches entity_count(). Freed slots have
    // generation == 0 and are skipped.
    for (std::uint32_t i = 0; i < slots_.size(); ++i) {
        const auto& slot = slots_[i];
        if (slot.generation != 0 && slot.archetype_id == kInvalidArchetypeId) {
            fn(Entity::from_parts(i, slot.generation));
        }
    }
}

const EntitySlot* World::slot_for(Entity e) const noexcept {
    if (!is_alive(e)) return nullptr;
    return &slots_[e.index()];
}

// ---------------------------------------------------------------------------
// Serialization helpers (ADR-0006 §2.9)
// ---------------------------------------------------------------------------

void World::clear() noexcept {
    // Walk every live entity in every archetype and run destructors on the
    // non-trivially-destructible components (mirrors ~World); then reset.
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        auto& arch = archetypes_.archetype(aid);
        for (std::size_t ci = 0; ci < arch.chunk_count(); ++ci) {
            auto& chunk = arch.chunk(ci);
            const auto live = chunk.live_count();
            for (std::uint16_t s = 0; s < live; ++s) {
                arch.destruct_components_at(registry_, static_cast<std::uint16_t>(ci), s);
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
    const Entity e{bits};
    if (e.generation() == 0) return Entity::null();
    const auto idx = e.index();
    // Grow the slot table if needed; freshly created slots have gen=0.
    if (idx >= slots_.size()) {
        slots_.resize(static_cast<std::size_t>(idx) + 1);
    }
    auto& slot = slots_[idx];
    // Must point at an empty slot — anything else would collide with a live
    // entity and is a caller bug.
    if (slot.generation != 0) return Entity::null();

    slot.generation   = e.generation();
    slot.archetype_id = kInvalidArchetypeId;
    slot.chunk_index  = std::numeric_limits<std::uint16_t>::max();
    slot.slot_index   = std::numeric_limits<std::uint16_t>::max();
    ++live_entity_count_;

    // Remove this index from the free list if it happens to be there.
    auto it = std::find(free_list_.begin(), free_list_.end(), idx);
    if (it != free_list_.end()) free_list_.erase(it);

    return e;
}

bool World::add_component_raw(Entity e, ComponentTypeId tid,
                               const void* src, std::size_t size) {
    if (!is_alive(e)) return false;
    if (tid == kInvalidComponentTypeId) return false;
    if (tid >= registry_.component_count()) return false;

    const auto& info = registry_.info(tid);
    if (size != info.size) return false;
    // Non-trivially-copyable types must go through add_component<T>; the
    // serialization reflection path lands with ADR-0006 §2.5.
    if (!info.trivially_copyable) return false;

    auto& slot = slots_[e.index()];
    ComponentMask new_mask;
    std::vector<ComponentTypeId> new_types;
    if (slot.archetype_id != kInvalidArchetypeId) {
        const auto& cur = archetypes_.archetype(slot.archetype_id);
        new_mask        = cur.mask();
        new_types       = cur.types();
    }
    if (new_mask.test(tid)) {
        auto* dst = raw_component_(e, tid);
        if (dst && src) std::memcpy(dst, src, size);
        return true;
    }
    new_mask.set(tid);
    new_types.push_back(tid);
    std::sort(new_types.begin(), new_types.end());

    auto* dst_raw = migrate_entity_(e, new_mask, new_types, tid);
    if (!dst_raw || !src) return false;
    std::memcpy(dst_raw, src, size);
    return true;
}

// ---------------------------------------------------------------------------
// Raw component access
// ---------------------------------------------------------------------------

std::byte* World::raw_component_(Entity e, ComponentTypeId tid) noexcept {
    if (!is_alive(e))                        return nullptr;
    const auto& slot = slots_[e.index()];
    if (slot.archetype_id == kInvalidArchetypeId) return nullptr;
    auto& arch = archetypes_.archetype(slot.archetype_id);
    return arch.component_ptr(registry_, tid, slot.chunk_index, slot.slot_index);
}

const std::byte* World::raw_component_(Entity e, ComponentTypeId tid) const noexcept {
    if (!is_alive(e))                        return nullptr;
    const auto& slot = slots_[e.index()];
    if (slot.archetype_id == kInvalidArchetypeId) return nullptr;
    const auto& arch = archetypes_.archetype(slot.archetype_id);
    return arch.component_ptr(registry_, tid, slot.chunk_index, slot.slot_index);
}

// ---------------------------------------------------------------------------
// Entity migration
// ---------------------------------------------------------------------------

std::byte* World::migrate_entity_(Entity                         e,
                                    ComponentMask                new_mask,
                                    const std::vector<ComponentTypeId>& new_types_sorted,
                                    ComponentTypeId                added_tid) {
    auto& slot = slots_[e.index()];

    // Look up / create destination archetype.
    const auto dst_aid = archetypes_.get_or_create(new_mask,
                                                    new_types_sorted,
                                                    registry_);
    auto& dst_arch = archetypes_.archetype(dst_aid);
    const auto dst_slot = dst_arch.allocate_slot();

    // Write the entity handle into the new slot.
    dst_arch.chunk(dst_slot.chunk_index).entity_at(dst_slot.slot) = e;

    // Move every *shared* component from source to destination.
    if (slot.archetype_id != kInvalidArchetypeId) {
        auto& src_arch   = archetypes_.archetype(slot.archetype_id);
        const auto sci   = slot.chunk_index;
        const auto ss    = slot.slot_index;

        for (auto tid : src_arch.types()) {
            if (!new_mask.test(tid)) continue;    // being removed
            const auto& info = registry_.info(tid);
            auto* src = src_arch.component_ptr(registry_, tid, sci, ss);
            auto* dst = dst_arch.component_ptr(registry_, tid, dst_slot.chunk_index, dst_slot.slot);
            if (info.trivially_copyable) {
                std::memcpy(dst, src, info.size);
            } else {
                info.move_construct(dst, src);
                if (info.destruct) info.destruct(src);
            }
        }

        // Destruct any components that are being *removed* (in src but not
        // in new_mask).
        for (auto tid : src_arch.types()) {
            if (new_mask.test(tid)) continue;
            const auto& info = registry_.info(tid);
            if (info.trivially_copyable) continue;
            auto* src = src_arch.component_ptr(registry_, tid, sci, ss);
            if (info.destruct) info.destruct(src);
        }

        // Compact source chunk (swap-back).
        const Entity swapped = src_arch.free_slot_swap_back(registry_, sci, ss);
        if (!swapped.is_null()) {
            auto& swapped_slot       = slots_[swapped.index()];
            swapped_slot.chunk_index = sci;
            swapped_slot.slot_index  = ss;
        }
    }

    // Update entity table row.
    slot.archetype_id = dst_aid;
    slot.chunk_index  = dst_slot.chunk_index;
    slot.slot_index   = dst_slot.slot;

    // If adding, return a pointer to the uninitialized destination for the
    // caller to placement-construct into.
    if (added_tid != kInvalidComponentTypeId) {
        return dst_arch.component_ptr(registry_, added_tid,
                                        dst_slot.chunk_index, dst_slot.slot);
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Hierarchy
// ---------------------------------------------------------------------------

namespace {

// Walk the sibling chain starting at `head`, invoking `fn(e)` for each.
// If `fn` returns false, stops early.
template <typename Fn>
void walk_siblings(const World& w, Entity head, Fn&& fn) {
    Entity cur = head;
    while (!cur.is_null()) {
        if (!fn(cur)) return;
        const auto* h = w.get_component<HierarchyComponent>(cur);
        if (!h) return;
        cur = h->next_sibling;
    }
}

// Returns true if `candidate` is `root` itself or one of its descendants.
bool is_descendant_or_self(const World& w, Entity root, Entity candidate) {
    if (root == candidate) return true;
    const auto* rh = w.get_component<HierarchyComponent>(root);
    if (!rh) return false;
    bool found = false;
    Entity child = rh->first_child;
    while (!child.is_null() && !found) {
        if (is_descendant_or_self(w, child, candidate)) {
            found = true;
            break;
        }
        const auto* ch = w.get_component<HierarchyComponent>(child);
        if (!ch) break;
        child = ch->next_sibling;
    }
    return found;
}

// Unlink `child` from its current parent's child list, if any. Callers must
// ensure `child` has a HierarchyComponent.
void unlink_from_parent(World& w, Entity child) {
    auto* ch = w.get_component<HierarchyComponent>(child);
    if (!ch || ch->parent.is_null()) return;

    auto* ph = w.get_component<HierarchyComponent>(ch->parent);
    if (!ph) {
        ch->parent = Entity::null();
        return;
    }
    if (ph->first_child == child) {
        ph->first_child = ch->next_sibling;
    } else {
        Entity cur = ph->first_child;
        while (!cur.is_null()) {
            auto* curh = w.get_component<HierarchyComponent>(cur);
            if (!curh) break;
            if (curh->next_sibling == child) {
                curh->next_sibling = ch->next_sibling;
                break;
            }
            cur = curh->next_sibling;
        }
    }
    ch->parent       = Entity::null();
    ch->next_sibling = Entity::null();
}

} // namespace

bool World::reparent(Entity child, Entity new_parent) {
    if (!is_alive(child)) return false;
    if (!new_parent.is_null() && !is_alive(new_parent)) return false;
    if (child == new_parent) return false;

    // Ensure child has a HierarchyComponent.
    auto* ch = get_component<HierarchyComponent>(child);
    if (!ch) {
        add_component<HierarchyComponent>(child, HierarchyComponent{});
        ch = get_component<HierarchyComponent>(child);
    }
    // Cycle check: new_parent must NOT be in the subtree rooted at child.
    if (!new_parent.is_null() && is_descendant_or_self(*this, child, new_parent)) {
        return false;
    }

    unlink_from_parent(*this, child);

    if (!new_parent.is_null()) {
        auto* ph = get_component<HierarchyComponent>(new_parent);
        if (!ph) {
            add_component<HierarchyComponent>(new_parent, HierarchyComponent{});
            ph = get_component<HierarchyComponent>(new_parent);
        }
        // Re-fetch child's hierarchy — add_component on new_parent may have
        // migrated the archetype table, which keeps `child`'s slot pointer
        // stable (different archetype) but don't assume `ch` is still valid
        // across arbitrary migrations; re-fetch to be safe.
        ch = get_component<HierarchyComponent>(child);
        ch->parent       = new_parent;
        ch->next_sibling = ph->first_child;
        ph->first_child  = child;
    }
    return true;
}

void World::visit_descendants(Entity root, const std::function<void(Entity)>& fn) const {
    if (!is_alive(root)) return;
    fn(root);
    const auto* rh = get_component<HierarchyComponent>(root);
    if (!rh) return;

    Entity cur = rh->first_child;
    while (!cur.is_null()) {
        visit_descendants(cur, fn);
        const auto* ch = get_component<HierarchyComponent>(cur);
        if (!ch) break;
        cur = ch->next_sibling;
    }
}

void World::visit_roots(const std::function<void(Entity)>& fn) const {
    // Every entity with a HierarchyComponent whose parent is null is a root.
    // Entities WITHOUT a HierarchyComponent are also implicit roots (they
    // participate in the scene as loose, parent-less entities).
    for (ArchetypeId aid = 0; aid < archetypes_.archetype_count(); ++aid) {
        const auto& arch = archetypes_.archetype(aid);
        const bool has_hierarchy = arch.mask().test(
            registry_.id_of_or_invalid<HierarchyComponent>());
        for (std::size_t ci = 0; ci < arch.chunk_count(); ++ci) {
            const auto& chunk = arch.chunk(ci);
            const auto live = chunk.live_count();
            for (std::uint16_t s = 0; s < live; ++s) {
                const Entity e = chunk.entity_at(s);
                if (has_hierarchy) {
                    const auto* h = get_component<HierarchyComponent>(e);
                    if (h && !h->parent.is_null()) continue;
                }
                fn(e);
            }
        }
    }
    // Archetype-less entities (no components) are also roots.
    for (std::uint32_t i = 0; i < slots_.size(); ++i) {
        const auto& slot = slots_[i];
        if (slot.generation != 0 && slot.archetype_id == kInvalidArchetypeId) {
            fn(Entity::from_parts(i, slot.generation));
        }
    }
}

std::vector<Entity> World::children_of(Entity e) const {
    std::vector<Entity> out;
    const auto* h = get_component<HierarchyComponent>(e);
    if (!h) return out;
    Entity cur = h->first_child;
    while (!cur.is_null()) {
        out.push_back(cur);
        const auto* ch = get_component<HierarchyComponent>(cur);
        if (!ch) break;
        cur = ch->next_sibling;
    }
    return out;
}

} // namespace ecs
} // namespace gw
