// editor/scene/blockout_physics_sync.cpp

#include "editor/scene/blockout_physics_sync.hpp"

#include "editor/scene/components.hpp"
#include "engine/physics/collider.hpp"
#include "engine/physics/physics_types.hpp"
#include "engine/physics/rigid_body.hpp"

#include <cmath>
#include <cstdint>

namespace gw::editor::scene {

std::uint32_t sync_blockout_boxes_to_physics(gw::physics::PhysicsWorld& phys,
                                             gw::ecs::World&          world) {
    if (!phys.initialized()) return 0;

    std::uint32_t created = 0;
    world.for_each<TransformComponent, BlockoutPrimitiveComponent>(
        [&](gw::ecs::Entity e, TransformComponent& t, BlockoutPrimitiveComponent& b) {
            if (b.shape != 0) return; // only box in v1
            const float hx =
                0.5f * std::max(0.01f, static_cast<float>(std::abs(t.scale.x)));
            const float hy =
                0.5f * std::max(0.01f, static_cast<float>(std::abs(t.scale.y)));
            const float hz =
                0.5f * std::max(0.01f, static_cast<float>(std::abs(t.scale.z)));

            const auto sh = phys.create_shape(gw::physics::BoxShapeDesc{
                .half_extents_m = {hx, hy, hz},
                .convex_radius_m = 0.02f,
            });
            if (!sh.valid()) return;

            gw::physics::RigidBodyDesc desc{};
            desc.shape       = sh;
            desc.motion_type = gw::physics::BodyMotionType::Static;
            desc.layer       = gw::physics::ObjectLayer::Static;
            desc.position_ws = t.position;
            desc.rotation_ws = t.rotation;
            desc.entity = static_cast<gw::physics::EntityId>(e.raw_bits());

            const auto body = phys.create_body(desc);
            if (body.valid()) ++created;
        });
    return created;
}

} // namespace gw::editor::scene
