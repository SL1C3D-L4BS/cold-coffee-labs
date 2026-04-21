#pragma once
// engine/physics/physics_world.hpp — Phase 12 Wave 12A (ADR-0031).
//
// PIMPL facade over the physics backend. The default backend is the null
// backend (always available). When `GW_PHYSICS_JOLT` is defined at compile
// time a real `jolt_bridge` backend is compiled in. Either way, **no Jolt
// headers are reachable from any caller of this file**.

#include "engine/physics/character_controller.hpp"
#include "engine/physics/collider.hpp"
#include "engine/physics/constraint.hpp"
#include "engine/physics/debug_draw.hpp"
#include "engine/physics/determinism_hash.hpp"
#include "engine/physics/events_physics.hpp"
#include "engine/physics/physics_config.hpp"
#include "engine/physics/physics_types.hpp"
#include "engine/physics/queries.hpp"
#include "engine/physics/rigid_body.hpp"

#include "engine/core/events/event_bus.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace gw::events { class CrossSubsystemBus; }
namespace gw::jobs { class Scheduler; }

namespace gw::physics {

enum class BackendKind : std::uint8_t {
    Null = 0,
    Jolt = 1,
};

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&)            = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&)                 = delete;
    PhysicsWorld& operator=(PhysicsWorld&&)      = delete;

    // Lifecycle ---------------------------------------------------------------
    [[nodiscard]] bool initialize(const PhysicsConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] BackendKind backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;

    // Configuration -----------------------------------------------------------
    [[nodiscard]] const PhysicsConfig& config() const noexcept;

    void set_gravity(const glm::vec3& g) noexcept;
    [[nodiscard]] glm::vec3 gravity() const noexcept;

    // Shape / body management -------------------------------------------------
    [[nodiscard]] ShapeHandle create_shape(const ShapeDesc& desc);
    void                     destroy_shape(ShapeHandle shape) noexcept;

    [[nodiscard]] BodyHandle create_body(const RigidBodyDesc& desc);
    void                     destroy_body(BodyHandle body) noexcept;

    void set_body_transform(BodyHandle body, const glm::dvec3& pos, const glm::quat& rot) noexcept;
    void set_body_linear_velocity(BodyHandle body, const glm::vec3& v) noexcept;
    void set_body_angular_velocity(BodyHandle body, const glm::vec3& v) noexcept;
    void apply_force(BodyHandle body, const glm::vec3& force_n) noexcept;
    void apply_impulse(BodyHandle body, const glm::vec3& impulse_ns) noexcept;
    void wake_body(BodyHandle body) noexcept;
    void put_to_sleep(BodyHandle body) noexcept;

    [[nodiscard]] BodyState body_state(BodyHandle body) const noexcept;
    [[nodiscard]] std::size_t body_count() const noexcept;
    [[nodiscard]] std::size_t active_body_count() const noexcept;

    // Constraints -------------------------------------------------------------
    [[nodiscard]] ConstraintHandle create_constraint(const ConstraintDesc& desc);
    void                           destroy_constraint(ConstraintHandle c) noexcept;

    // Vehicle (optional) ------------------------------------------------------
    [[nodiscard]] VehicleHandle create_vehicle(const VehicleDesc& desc);
    void                        destroy_vehicle(VehicleHandle v) noexcept;
    void                        set_vehicle_input(VehicleHandle v, const VehicleInput& input) noexcept;

    // Characters --------------------------------------------------------------
    [[nodiscard]] CharacterHandle create_character(const CharacterDesc& desc);
    void                          destroy_character(CharacterHandle c) noexcept;
    void                          set_character_input(CharacterHandle c, const CharacterInput& input) noexcept;
    [[nodiscard]] CharacterState  character_state(CharacterHandle c) const noexcept;

    // Simulation --------------------------------------------------------------
    // Advance the world by `delta_time_s` of wall-clock; the backend folds
    // time into fixed steps of `config.fixed_dt`, capped at
    // `config.max_substeps`. Returns the number of fixed steps executed.
    [[nodiscard]] std::uint32_t step(float delta_time_s);
    // Advance exactly one fixed step unconditionally.
    void step_fixed();
    // Interpolation alpha for rendering between the previous and current
    // fixed step (0..1). Safe to call on any thread.
    [[nodiscard]] float interpolation_alpha() const noexcept;

    void reset();
    void pause(bool paused) noexcept;
    [[nodiscard]] bool paused() const noexcept;

    [[nodiscard]] std::uint64_t frame_index() const noexcept;

    // Queries -----------------------------------------------------------------
    [[nodiscard]] bool raycast_closest(const RaycastInput& in,
                                       const QueryFilter& filter,
                                       RaycastHit& out) const;
    [[nodiscard]] bool raycast_any(const RaycastInput& in,
                                   const QueryFilter& filter,
                                   RaycastHit& out) const;

    // Batched raycasts — may dispatch to `jobs::Scheduler` if provided. All
    // callers are blocked until all results are filled.
    void raycast_batch(const RaycastBatch& batch, const QueryFilter& filter) const;

    [[nodiscard]] bool shape_sweep_closest(const ShapeSweepInput& in,
                                           const QueryFilter& filter,
                                           ShapeQueryHit& out) const;

    [[nodiscard]] std::size_t overlap(const OverlapInput& in,
                                      const QueryFilter& filter,
                                      std::span<ShapeQueryHit> out) const;

    // Determinism -------------------------------------------------------------
    [[nodiscard]] std::uint64_t determinism_hash(const HashOptions& opt = {}) const;
    void collect_hash_entries(std::vector<HashBodyEntry>& out) const;

    // Debug draw --------------------------------------------------------------
    void set_debug_flag_mask(std::uint32_t mask) noexcept;
    [[nodiscard]] std::uint32_t debug_flag_mask() const noexcept;
    void fill_debug_draw(DebugDrawSink& sink) const;

    // Event buses (owned) -----------------------------------------------------
    [[nodiscard]] events::EventBus<events::PhysicsContactAdded>&     bus_contact_added()     noexcept;
    [[nodiscard]] events::EventBus<events::PhysicsContactPersisted>& bus_contact_persisted() noexcept;
    [[nodiscard]] events::EventBus<events::PhysicsContactRemoved>&   bus_contact_removed()   noexcept;
    [[nodiscard]] events::EventBus<events::TriggerEntered>&          bus_trigger_entered()   noexcept;
    [[nodiscard]] events::EventBus<events::TriggerExited>&           bus_trigger_exited()    noexcept;
    [[nodiscard]] events::EventBus<events::CharacterGrounded>&       bus_character_grounded() noexcept;
    [[nodiscard]] events::EventBus<events::ConstraintBroken>&        bus_constraint_broken() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::physics
