// editor/scene/transform_system.cpp
#include "transform_system.hpp"
#include "components.hpp"

#include "engine/ecs/hierarchy.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

namespace gw::editor::scene {

namespace {

// Compose a local-space dmat4 from a TransformComponent. Scale is promoted to
// double so the result is precision-clean; rotation is converted via
// glm::mat4_cast then widened.
glm::dmat4 local_matrix(const TransformComponent& t) {
    // Translation × Rotation × Scale, standard TRS.
    const glm::dmat4 tr = glm::translate(glm::dmat4{1.0}, t.position);
    const glm::dmat4 ro = glm::dmat4(glm::mat4_cast(t.rotation));
    const glm::dmat4 sc = glm::scale(glm::dmat4{1.0},
                                      glm::dvec3(t.scale));
    return tr * ro * sc;
}

} // namespace

std::size_t update_transforms(gw::ecs::World& w) {
    using gw::ecs::Entity;
    using gw::ecs::HierarchyComponent;

    // Ensure WorldMatrixComponent is registered; consumers may not have
    // touched it yet.
    (void)w.component_registry().id_of<WorldMatrixComponent>();

    // First pass: collect (entity, local_matrix, parent) triples for each
    // entity that has a TransformComponent. The iteration is driven by
    // for_each<TransformComponent> so archetype jump is cache-friendly.
    //
    // thread_local keeps the vectors hot across frames; editor workloads
    // rarely need more than a few hundred transforms.
    struct Record {
        Entity     entity;
        Entity     parent;
        glm::dmat4 local;
    };

    static thread_local std::vector<Record> records;
    records.clear();

    w.for_each<TransformComponent>([&](Entity e, TransformComponent& t) {
        Entity parent = Entity::null();
        if (const auto* h = w.get_component<HierarchyComponent>(e)) {
            parent = h->parent;
        }
        records.push_back(Record{e, parent, local_matrix(t)});
    });

    if (records.empty()) return 0;

    // Second pass: compose world matrices. We implement a two-pass fixed
    // point: iterate until every record has a world matrix (topological sort
    // in O(n·d) where d is tree depth). For editor workloads d is tiny; a
    // proper topo-sort can come in Phase 9 once depth starts mattering.
    const std::size_t n = records.size();
    std::vector<std::uint8_t>  resolved(n, 0);
    std::vector<glm::dmat4>    world(n);

    auto index_of = [&](Entity e) -> std::size_t {
        // Linear probe — n is small; cache-friendlier than an unordered_map
        // for the scales we care about (<4k entities).
        for (std::size_t i = 0; i < n; ++i) {
            if (records[i].entity.bits == e.bits) return i;
        }
        return static_cast<std::size_t>(-1);
    };

    std::size_t remaining = n;
    // At most `n` outer passes: every pass resolves at least one node
    // (the topologically-deepest unresolved root of the working set).
    for (std::size_t pass = 0; pass < n && remaining > 0; ++pass) {
        bool progress = false;
        for (std::size_t i = 0; i < n; ++i) {
            if (resolved[i]) continue;
            const auto& rec = records[i];
            if (rec.parent.is_null()) {
                world[i]    = rec.local;
                resolved[i] = 1;
                --remaining;
                progress    = true;
                continue;
            }
            const auto pi = index_of(rec.parent);
            if (pi == static_cast<std::size_t>(-1)) {
                // Parent entity has no TransformComponent — treat as identity
                // root so an orphan in the hierarchy still gets a world
                // matrix rather than being silently skipped.
                world[i]    = rec.local;
                resolved[i] = 1;
                --remaining;
                progress    = true;
                continue;
            }
            if (!resolved[pi]) continue;
            world[i]    = world[pi] * rec.local;
            resolved[i] = 1;
            --remaining;
            progress    = true;
        }
        if (!progress) break;  // cycle — should be impossible if reparent()
                                // guards, but don't hang if it does.
    }

    // Third pass: write WorldMatrixComponent for every resolved entity.
    std::size_t written = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (!resolved[i]) continue;
        auto* wm = w.get_component<WorldMatrixComponent>(records[i].entity);
        if (!wm) {
            wm = &w.add_component(records[i].entity, WorldMatrixComponent{});
        }
        wm->world = world[i];
        wm->dirty = 0;
        ++written;
    }
    return written;
}

} // namespace gw::editor::scene
