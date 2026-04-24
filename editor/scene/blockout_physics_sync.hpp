#pragma once
// editor/scene/blockout_physics_sync.hpp — greybox primitives → PhysicsWorld bodies (Phase 24 parity).

#include "engine/ecs/world.hpp"
#include "engine/physics/physics_world.hpp"

#include <cstdint>

namespace gw::editor::scene {

/// Creates **static** box bodies for entities that have both `TransformComponent`
/// and `BlockoutPrimitiveComponent` with `shape == 0` (box). Half-extents come
/// from `TransformComponent::scale`. Returns number of bodies created.
[[nodiscard]] std::uint32_t sync_blockout_boxes_to_physics(gw::physics::PhysicsWorld& phys,
                                                           gw::ecs::World&          world);

} // namespace gw::editor::scene
