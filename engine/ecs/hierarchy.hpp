#pragma once
// engine/ecs/hierarchy.hpp
// HierarchyComponent — scene-graph parent/sibling/child linkage.
// See docs/adr/0004-ecs-world.md §2.7.

#include "entity.hpp"

namespace gw {
namespace ecs {

// Intrusive linked-list hierarchy node. The World implements traversal and
// reparent helpers that maintain these fields; do not edit them directly
// (they must stay a well-formed scene graph).
struct HierarchyComponent {
    Entity parent       = Entity::null();
    Entity first_child  = Entity::null();
    Entity next_sibling = Entity::null();
};

static_assert(sizeof(HierarchyComponent) == 3 * sizeof(Entity));

} // namespace ecs
} // namespace gw
