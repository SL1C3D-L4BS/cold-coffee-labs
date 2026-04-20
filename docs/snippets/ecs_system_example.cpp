// snippets/ecs_system_example.cpp — Greywater_Engine
//
// The canonical ECS system shape. Systems are plain functions with declared
// read/write component sets; the scheduler runs non-conflicting systems in
// parallel without locks. This snippet demonstrates:
//
//   - Component types are plain data (no virtuals, no constructors that allocate).
//   - Queries are compile-time-typed spans over SoA-laid-out archetype chunks.
//   - Systems mutate through explicit read/write tags so the scheduler can reason.
//   - No STL in the hot path — use `gw::core::span`, not `std::span`.
//   - No raw `std::thread` — all parallelism goes through `gw::jobs`.
//
// Place actual runtime systems under `engine/<subsystem>/` and register them
// via `World::register_system<...>()` at startup.

#include <gw/core/span.hpp>
#include <gw/ecs/world.hpp>
#include <gw/math/vec.hpp>

namespace gw::gameplay {

// Plain-data components. No virtuals, no allocations, trivially-copyable.
struct Transform {
    math::Vec3 position;
    math::Quat rotation;
    math::Vec3 scale;
};

struct Velocity {
    math::Vec3 linear;
    math::Vec3 angular;
};

// A system: plain function, explicit read/write declarations via the query.
// `Write<Transform>` + `Read<Velocity>` tells the scheduler what this system
// touches so it can parallelize without locks.
void integrate_velocity(
    ecs::Query<ecs::Write<Transform>, ecs::Read<Velocity>> query,
    f32 dt) noexcept
{
    for (auto [transform, velocity] : query) {
        transform.position += velocity.linear * dt;
        // Quaternion integration via small-angle approximation omitted for brevity.
    }
}

// Registration happens once, typically at subsystem init.
inline void register_gameplay_systems(ecs::World& world)
{
    world.register_system(integrate_velocity);
}

} // namespace gw::gameplay
