// editor/scene/transform_system.cpp
#include "transform_system.hpp"
#include "components.hpp"

#include "engine/ecs/hierarchy.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <deque>
#include <unordered_map>
#include <vector>

namespace gw::editor::scene {

namespace {

glm::dmat4 local_matrix(const TransformComponent& t) {
    const glm::dmat4 tr = glm::translate(glm::dmat4{1.0}, t.position);
    const glm::dmat4 ro = glm::dmat4(glm::mat4_cast(t.rotation));
    const glm::dmat4 sc = glm::scale(glm::dmat4{1.0}, glm::dvec3(t.scale));
    return tr * ro * sc;
}

} // namespace

std::size_t update_transforms(gw::ecs::World& w) {
    using gw::ecs::Entity;
    using gw::ecs::HierarchyComponent;

    (void)w.component_registry().id_of<WorldMatrixComponent>();

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

    const std::size_t n = records.size();
    std::unordered_map<std::uint64_t, std::size_t> entity_to_index;
    entity_to_index.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
        entity_to_index[records[i].entity.raw_bits()] = i;
    }

    std::vector<std::vector<std::size_t>> children(n);
    std::vector<int>                    indegree(n, 0);

    for (std::size_t i = 0; i < n; ++i) {
        const Entity p = records[i].parent;
        if (p.is_null()) {
            indegree[i] = 0;
            continue;
        }
        const auto pit = entity_to_index.find(p.raw_bits());
        if (pit == entity_to_index.end()) {
            // Parent has no Transform row — treat as root for ordering.
            indegree[i] = 0;
            continue;
        }
        indegree[i] = 1;
        children[pit->second].push_back(i);
    }

    std::deque<std::size_t> q;
    for (std::size_t i = 0; i < n; ++i) {
        if (indegree[i] == 0) q.push_back(i);
    }

    std::vector<glm::dmat4> world(n);
    std::vector<std::uint8_t> resolved(n, 0);

    while (!q.empty()) {
        const std::size_t i = q.front();
        q.pop_front();

        const Record& rec = records[i];
        if (rec.parent.is_null()) {
            world[i] = rec.local;
        } else {
            const auto pit = entity_to_index.find(rec.parent.raw_bits());
            if (pit == entity_to_index.end() || !resolved[pit->second]) {
                world[i] = rec.local;
            } else {
                world[i] = world[pit->second] * rec.local;
            }
        }
        resolved[i] = 1;

        for (const std::size_t c : children[i]) {
            --indegree[c];
            if (indegree[c] == 0) q.push_back(c);
        }
    }

    // Cycles (should not happen with guarded reparent): fall back to local.
    for (std::size_t i = 0; i < n; ++i) {
        if (!resolved[i]) {
            world[i]    = records[i].local;
            resolved[i] = 1;
        }
    }

    std::size_t written = 0;
    for (std::size_t i = 0; i < n; ++i) {
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
