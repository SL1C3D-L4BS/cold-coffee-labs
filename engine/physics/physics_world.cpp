// engine/physics/physics_world.cpp — Phase 12 (ADR-0031..0038).
//
// Facade implementation. The PIMPL dispatches to the null backend by
// default; when `GW_PHYSICS_JOLT` is defined a Jolt bridge takes over
// via link-time substitution of the `make_*_backend()` factories below.
// For Phase 12 we ship the deterministic null backend everywhere.

#include "engine/physics/physics_world.hpp"
#include "engine/physics/events_physics.hpp"

#include "engine/core/events/event_bus.hpp"
#include "engine/jobs/scheduler.hpp"

#include <glm/geometric.hpp>
#include <glm/common.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gw::physics {

// ---------------------------------------------------------------------------
// Shape record (null backend)
// ---------------------------------------------------------------------------
struct NullShape {
    ShapeDesc    desc;
    glm::vec3    local_aabb_min{-0.5f};
    glm::vec3    local_aabb_max{ 0.5f};
    // Static mesh shapes bake their triangle list here for raycasts.
    std::vector<glm::vec3>     tri_vertices{};
    std::vector<std::uint32_t> tri_indices{};
};

// ---------------------------------------------------------------------------
// Body record
// ---------------------------------------------------------------------------
struct NullBody {
    BodyState      state{};
    ShapeHandle    shape{};
    ObjectLayer    layer{ObjectLayer::Dynamic};
    EntityId       entity{kEntityNone};
    float          mass_kg{1.0f};
    float          friction{0.5f};
    float          restitution{0.0f};
    float          linear_damping{0.05f};
    float          angular_damping{0.05f};
    float          gravity_factor{1.0f};
    bool           is_sensor{false};
    bool           allow_sleeping{true};
    bool           continuous_collision{false};
    bool           alive{false};
    std::uint32_t  id{0};                // stable 1-based id (hashed)
    std::uint32_t  generation{0};
    glm::vec3      accumulated_force{0.0f};
    glm::vec3      accumulated_impulse{0.0f};
    float          idle_time{0.0f};      // seconds below sleep threshold
};

// ---------------------------------------------------------------------------
// Constraint record
// ---------------------------------------------------------------------------
struct NullConstraint {
    ConstraintDesc desc;
    bool           alive{false};
    bool           broken{false};
    float          accumulated_impulse{0.0f};
    float          break_impulse{0.0f};
    BodyHandle     a{};
    BodyHandle     b{};
    EntityId       entity_a{kEntityNone};
    EntityId       entity_b{kEntityNone};
};

// ---------------------------------------------------------------------------
// Character record
// ---------------------------------------------------------------------------
struct NullCharacter {
    CharacterDesc  desc{};
    CharacterState state{};
    CharacterInput pending_input{};
    float          coyote_time_s{0.0f};       // remaining coyote window
    float          time_since_grounded_s{0.0f};
    float          time_in_jump_s{0.0f};
    bool           alive{false};
    std::uint32_t  generation{0};
};

// ---------------------------------------------------------------------------
// Vehicle record (parking lot — full sim lives in Jolt build)
// ---------------------------------------------------------------------------
struct NullVehicle {
    VehicleDesc  desc{};
    VehicleInput input{};
    bool         alive{false};
};

// ---------------------------------------------------------------------------
// Contact bookkeeping for events
// ---------------------------------------------------------------------------
struct ContactKey {
    std::uint32_t a{0};
    std::uint32_t b{0};
    [[nodiscard]] constexpr bool operator==(const ContactKey& rhs) const noexcept {
        return a == rhs.a && b == rhs.b;
    }
};

struct ContactKeyHash {
    std::size_t operator()(const ContactKey& k) const noexcept {
        return (static_cast<std::size_t>(k.a) * 0x9E3779B97F4A7C15ULL) ^
               (static_cast<std::size_t>(k.b) << 1);
    }
};

// ---------------------------------------------------------------------------
// Helpers — AABB math for the null backend.
// ---------------------------------------------------------------------------
namespace {

glm::vec3 shape_half_extents_local(const ShapeDesc& d, glm::vec3& center_out) {
    center_out = glm::vec3{0.0f};
    if (auto* b = std::get_if<BoxShapeDesc>(&d))         return b->half_extents_m + glm::vec3{b->convex_radius_m};
    if (auto* s = std::get_if<SphereShapeDesc>(&d))      return glm::vec3{s->radius_m};
    if (auto* c = std::get_if<CapsuleShapeDesc>(&d))     return glm::vec3{c->radius_m, c->half_height_m + c->radius_m, c->radius_m};
    if (auto* c = std::get_if<CylinderShapeDesc>(&d))    return glm::vec3{c->radius_m, c->half_height_m, c->radius_m};
    if (auto* h = std::get_if<ConvexHullShapeDesc>(&d)) {
        glm::vec3 lo{ std::numeric_limits<float>::infinity()};
        glm::vec3 hi{-std::numeric_limits<float>::infinity()};
        for (const auto& p : h->points) {
            lo = glm::min(lo, p);
            hi = glm::max(hi, p);
        }
        center_out = 0.5f * (lo + hi);
        return 0.5f * (hi - lo);
    }
    if (auto* m = std::get_if<TriangleMeshShapeDesc>(&d)) {
        glm::vec3 lo{ std::numeric_limits<float>::infinity()};
        glm::vec3 hi{-std::numeric_limits<float>::infinity()};
        for (const auto& p : m->vertices) {
            lo = glm::min(lo, p);
            hi = glm::max(hi, p);
        }
        center_out = 0.5f * (lo + hi);
        return 0.5f * (hi - lo);
    }
    if (std::holds_alternative<CompoundShapeDesc>(d)) {
        return glm::vec3{0.5f}; // compound is a bookkeeping construct in null backend
    }
    return glm::vec3{0.5f};
}

// Ray vs AABB (slab method). Returns hit t-fraction in [0, 1].
bool ray_vs_aabb(const glm::vec3& origin, const glm::vec3& dir,
                 const glm::vec3& bmin, const glm::vec3& bmax,
                 float max_dist, float& t_out, glm::vec3& normal_out) {
    float tmin = 0.0f;
    float tmax = max_dist;
    glm::vec3 normal{0.0f};
    int hit_axis = -1;
    float hit_sign = 0.0f;

    for (int axis = 0; axis < 3; ++axis) {
        const float d = dir[axis];
        const float o = origin[axis];
        if (std::fabs(d) < 1e-8f) {
            if (o < bmin[axis] || o > bmax[axis]) return false;
        } else {
            float t1 = (bmin[axis] - o) / d;
            float t2 = (bmax[axis] - o) / d;
            float sign = 1.0f;
            if (t1 > t2) { std::swap(t1, t2); sign = -1.0f; }
            if (t1 > tmin) { tmin = t1; hit_axis = axis; hit_sign = sign; }
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }
    if (tmin < 0.0f || tmin > max_dist) return false;
    t_out = tmin;
    normal = glm::vec3{0.0f};
    if (hit_axis >= 0) normal[hit_axis] = -hit_sign;
    normal_out = normal;
    return true;
}

// AABB vs AABB overlap.
bool aabb_vs_aabb(const glm::vec3& a_min, const glm::vec3& a_max,
                  const glm::vec3& b_min, const glm::vec3& b_max) {
    return a_min.x <= b_max.x && a_max.x >= b_min.x
        && a_min.y <= b_max.y && a_max.y >= b_min.y
        && a_min.z <= b_max.z && a_max.z >= b_min.z;
}

// Ray-triangle (Möller–Trumbore).
bool ray_vs_triangle(const glm::vec3& origin, const glm::vec3& dir,
                     const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                     float max_dist, float& t_out, glm::vec3& normal_out) {
    const glm::vec3 e1 = v1 - v0;
    const glm::vec3 e2 = v2 - v0;
    const glm::vec3 p  = glm::cross(dir, e2);
    const float det = glm::dot(e1, p);
    if (std::fabs(det) < 1e-8f) return false;
    const float inv = 1.0f / det;
    const glm::vec3 s = origin - v0;
    const float u = glm::dot(s, p) * inv;
    if (u < 0.0f || u > 1.0f) return false;
    const glm::vec3 q = glm::cross(s, e1);
    const float v = glm::dot(dir, q) * inv;
    if (v < 0.0f || (u + v) > 1.0f) return false;
    const float t = glm::dot(e2, q) * inv;
    if (t < 0.0f || t > max_dist) return false;
    t_out = t;
    const glm::vec3 n = glm::normalize(glm::cross(e1, e2));
    normal_out = n;
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// PhysicsWorld::Impl — the null backend.
// ---------------------------------------------------------------------------
struct PhysicsWorld::Impl {
    PhysicsConfig                  cfg{};
    events::CrossSubsystemBus*     cross_bus{nullptr};
    jobs::Scheduler*               scheduler{nullptr};

    // Storage. Index 0 is "null"; handles are 1-based.
    std::vector<NullShape>         shapes{1};
    std::vector<NullBody>          bodies{1};
    std::vector<NullConstraint>    constraints{1};
    std::vector<NullCharacter>     characters{1};
    std::vector<NullVehicle>       vehicles{1};

    std::vector<std::uint32_t>     free_bodies{};
    std::vector<std::uint32_t>     free_shapes{};
    std::vector<std::uint32_t>     free_constraints{};
    std::vector<std::uint32_t>     free_characters{};
    std::vector<std::uint32_t>     free_vehicles{};

    // Previous frame contact set — for "added" / "removed" diffs.
    std::unordered_map<ContactKey, bool, ContactKeyHash> last_contacts{};

    // Event buses (owned). Consumers subscribe by subsystem wiring.
    events::EventBus<events::PhysicsContactAdded>     bus_contact_added{};
    events::EventBus<events::PhysicsContactPersisted> bus_contact_persisted{};
    events::EventBus<events::PhysicsContactRemoved>   bus_contact_removed{};
    events::EventBus<events::TriggerEntered>          bus_trigger_entered{};
    events::EventBus<events::TriggerExited>           bus_trigger_exited{};
    events::EventBus<events::CharacterGrounded>       bus_char_grounded{};
    events::EventBus<events::ConstraintBroken>        bus_constraint_broken{};

    // Simulation state.
    bool          initialized_flag{false};
    bool          paused_flag{false};
    float         time_accumulator_s{0.0f};
    std::uint64_t frame_index{0};
    std::uint32_t debug_flag_mask{0};

    // ---- accessors ----
    [[nodiscard]] NullBody* body_ptr(BodyHandle h) noexcept {
        if (!h.valid() || h.id >= bodies.size()) return nullptr;
        auto& b = bodies[h.id];
        return b.alive ? &b : nullptr;
    }
    [[nodiscard]] const NullBody* body_ptr(BodyHandle h) const noexcept {
        if (!h.valid() || h.id >= bodies.size()) return nullptr;
        const auto& b = bodies[h.id];
        return b.alive ? &b : nullptr;
    }
    [[nodiscard]] NullShape* shape_ptr(ShapeHandle h) noexcept {
        if (!h.valid() || h.id >= shapes.size()) return nullptr;
        return &shapes[h.id];
    }
    [[nodiscard]] const NullShape* shape_ptr(ShapeHandle h) const noexcept {
        if (!h.valid() || h.id >= shapes.size()) return nullptr;
        return &shapes[h.id];
    }

    // ---- AABB query ----
    void body_aabb(const NullBody& b, glm::vec3& lo, glm::vec3& hi) const {
        const NullShape* s = shape_ptr(b.shape);
        glm::vec3 centre_local{0.0f};
        glm::vec3 half{0.5f};
        if (s) half = shape_half_extents_local(s->desc, centre_local);
        const glm::vec3 centre_ws{
            static_cast<float>(b.state.position_ws.x),
            static_cast<float>(b.state.position_ws.y),
            static_cast<float>(b.state.position_ws.z)};
        lo = centre_ws + centre_local - half;
        hi = centre_ws + centre_local + half;
    }

    // ---- Contact pair handling ----
    static ContactKey key_for(std::uint32_t a, std::uint32_t b) noexcept {
        return (a < b) ? ContactKey{a, b} : ContactKey{b, a};
    }

    bool layer_collides(ObjectLayer la, ObjectLayer lb) const noexcept {
        if (la == ObjectLayer::Static && lb == ObjectLayer::Static) return false;
        if (la == ObjectLayer::Trigger || lb == ObjectLayer::Trigger) return true; // triggers still report
        return true;
    }

    void step_integrate(float dt) {
        const glm::vec3 g = cfg.gravity;
        for (auto& b : bodies) {
            if (!b.alive) continue;
            if (b.state.motion_type != BodyMotionType::Dynamic) continue;

            glm::vec3 acc = g * b.gravity_factor;
            if (b.mass_kg > 0.0f) {
                acc += b.accumulated_force / b.mass_kg;
                b.state.linear_velocity += b.accumulated_impulse / b.mass_kg;
            }
            b.accumulated_force  = glm::vec3{0.0f};
            b.accumulated_impulse = glm::vec3{0.0f};

            b.state.linear_velocity += acc * dt;
            b.state.linear_velocity *= (1.0f - b.linear_damping * dt);
            b.state.angular_velocity *= (1.0f - b.angular_damping * dt);

            glm::dvec3 delta{
                static_cast<double>(b.state.linear_velocity.x) * dt,
                static_cast<double>(b.state.linear_velocity.y) * dt,
                static_cast<double>(b.state.linear_velocity.z) * dt};
            b.state.position_ws += delta;

            // Sleep bookkeeping — required for deterministic hash stability
            // across cross-platform replays once the world quiesces.
            const float lin_mag2 = glm::dot(b.state.linear_velocity, b.state.linear_velocity);
            const float ang_mag2 = glm::dot(b.state.angular_velocity, b.state.angular_velocity);
            const float lin_eps2 = cfg.sleep_linear_threshold * cfg.sleep_linear_threshold;
            const float ang_eps2 = cfg.sleep_angular_threshold * cfg.sleep_angular_threshold;
            if (b.allow_sleeping && lin_mag2 < lin_eps2 && ang_mag2 < ang_eps2) {
                b.idle_time += dt;
                if (b.idle_time > 0.5f) {
                    b.state.awake = false;
                    b.state.linear_velocity  = glm::vec3{0.0f};
                    b.state.angular_velocity = glm::vec3{0.0f};
                }
            } else {
                b.idle_time = 0.0f;
                b.state.awake = true;
            }
        }
    }

    void step_resolve_collisions(float /*dt*/) {
        const std::size_t n = bodies.size();
        std::unordered_map<ContactKey, bool, ContactKeyHash> current_contacts;
        current_contacts.reserve(n);

        for (std::size_t i = 1; i < n; ++i) {
            auto& a = bodies[i];
            if (!a.alive) continue;
            glm::vec3 a_lo, a_hi;
            body_aabb(a, a_lo, a_hi);
            for (std::size_t j = i + 1; j < n; ++j) {
                auto& b = bodies[j];
                if (!b.alive) continue;
                if (a.state.motion_type == BodyMotionType::Static &&
                    b.state.motion_type == BodyMotionType::Static) continue;
                if (!layer_collides(a.layer, b.layer)) continue;
                glm::vec3 b_lo, b_hi;
                body_aabb(b, b_lo, b_hi);
                if (!aabb_vs_aabb(a_lo, a_hi, b_lo, b_hi)) continue;

                const ContactKey key = key_for(a.id, b.id);
                current_contacts[key] = true;

                if (a.layer == ObjectLayer::Trigger || b.layer == ObjectLayer::Trigger ||
                    a.is_sensor || b.is_sensor) {
                    const bool was = last_contacts.contains(key);
                    if (!was) {
                        events::TriggerEntered ev{};
                        ev.trigger = (a.layer == ObjectLayer::Trigger || a.is_sensor) ? a.entity : b.entity;
                        ev.other   = (a.layer == ObjectLayer::Trigger || a.is_sensor) ? b.entity : a.entity;
                        bus_trigger_entered.publish(ev);
                    }
                    continue;
                }

                // Compute penetration vector (smallest overlap axis).
                const glm::vec3 centre_a = 0.5f * (a_lo + a_hi);
                const glm::vec3 centre_b = 0.5f * (b_lo + b_hi);
                const glm::vec3 half_a   = 0.5f * (a_hi - a_lo);
                const glm::vec3 half_b   = 0.5f * (b_hi - b_lo);
                const glm::vec3 d = centre_b - centre_a;
                const glm::vec3 overlap = (half_a + half_b) - glm::abs(d);
                if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) continue;

                glm::vec3 normal{0.0f};
                float depth = overlap.x;
                int axis = 0;
                if (overlap.y < depth) { depth = overlap.y; axis = 1; }
                if (overlap.z < depth) { depth = overlap.z; axis = 2; }
                normal[axis] = (d[axis] >= 0.0f) ? 1.0f : -1.0f;

                // Resolve only the dynamic side(s).
                const bool a_dyn = a.state.motion_type == BodyMotionType::Dynamic;
                const bool b_dyn = b.state.motion_type == BodyMotionType::Dynamic;
                if (a_dyn && b_dyn) {
                    a.state.position_ws -= glm::dvec3(normal * (depth * 0.5f));
                    b.state.position_ws += glm::dvec3(normal * (depth * 0.5f));
                } else if (a_dyn) {
                    a.state.position_ws -= glm::dvec3(normal * depth);
                } else if (b_dyn) {
                    b.state.position_ws += glm::dvec3(normal * depth);
                }

                // Zero out the velocity along the contact normal (inelastic).
                auto zero_normal_v = [&](NullBody& body, const glm::vec3& n) {
                    const float vn = glm::dot(body.state.linear_velocity, n);
                    if (vn < 0.0f) {
                        body.state.linear_velocity -= vn * n;
                    }
                };
                if (a_dyn) zero_normal_v(a, -normal);
                if (b_dyn) zero_normal_v(b,  normal);

                const events::PhysicsContactAdded add{
                    a.entity, b.entity,
                    0.5f * (centre_a + centre_b),
                    normal,
                    depth * 1000.0f,  // placeholder impulse proxy
                };
                const bool was = last_contacts.contains(key);
                if (!was) {
                    bus_contact_added.publish(add);
                } else {
                    events::PhysicsContactPersisted p{
                        add.a, add.b, add.point_ws, add.normal,
                    };
                    bus_contact_persisted.publish(p);
                }
            }
        }

        // Emit "removed" events for pairs that existed last frame but not now.
        for (const auto& [key, _] : last_contacts) {
            if (!current_contacts.contains(key)) {
                events::PhysicsContactRemoved rem{};
                if (key.a < bodies.size() && bodies[key.a].alive) rem.a = bodies[key.a].entity;
                if (key.b < bodies.size() && bodies[key.b].alive) rem.b = bodies[key.b].entity;
                bus_contact_removed.publish(rem);
            }
        }

        last_contacts = std::move(current_contacts);
    }

    void step_constraints(float dt) {
        (void)dt;
        for (auto& c : constraints) {
            if (!c.alive || c.broken) continue;
            if (c.break_impulse <= 0.0f) continue;
            // Derive a proxy impulse from the relative contact velocity of the
            // two connected bodies — deterministic, platform-independent.
            const NullBody* a = body_ptr(c.a);
            const NullBody* b = body_ptr(c.b);
            if (!a || !b) continue;
            const glm::vec3 rel_v = b->state.linear_velocity - a->state.linear_velocity;
            c.accumulated_impulse = glm::length(rel_v);
            if (c.accumulated_impulse >= c.break_impulse) {
                c.broken = true;
                events::ConstraintBroken ev{c.entity_a, c.entity_b, c.accumulated_impulse};
                bus_constraint_broken.publish(ev);
            }
        }
    }

    void step_characters(float dt) {
        const glm::vec3 g = cfg.gravity;
        const float slope_max_cos = std::cos(glm::radians(std::max(0.1f, 90.0f - 0.1f)));
        (void)slope_max_cos;

        for (auto& c : characters) {
            if (!c.alive) continue;
            const float slope_cos = std::cos(glm::radians(c.desc.slope_max_angle_deg));

            // Apply input (horizontal velocity target + vertical from gravity).
            glm::vec3 v = c.pending_input.move_velocity_ws;
            v.y = c.state.velocity.y;

            // Jump (consumes coyote window).
            const bool can_jump = c.state.on_ground || c.state.in_coyote_window;
            if (c.pending_input.jump_pressed && can_jump) {
                v.y = c.desc.jump_velocity;
                c.state.on_ground = false;
                c.state.in_coyote_window = false;
                c.coyote_time_s = 0.0f;
                c.time_in_jump_s = 0.0f;
            }

            // Gravity.
            v.y += g.y * c.desc.gravity_scale * dt;

            // Integrate.
            c.state.position_ws += glm::dvec3{
                static_cast<double>(v.x) * dt,
                static_cast<double>(v.y) * dt,
                static_cast<double>(v.z) * dt};

            // Ground probe — raycast straight down, find closest hit.
            const glm::vec3 origin{
                static_cast<float>(c.state.position_ws.x),
                static_cast<float>(c.state.position_ws.y),
                static_cast<float>(c.state.position_ws.z)};
            const glm::vec3 dir{0.0f, -1.0f, 0.0f};
            const float probe_dist = c.desc.capsule_half_height_m + c.desc.capsule_radius_m + c.desc.step_down_max_m;

            RaycastInput rin{};
            rin.origin_ws    = origin;
            rin.direction_ws = dir;
            rin.max_distance_m = probe_dist;

            RaycastHit hit{};
            const bool grounded_raw = raycast_closest_impl(rin, QueryFilter{}, hit, /*exclude_trigger=*/true);

            const bool was_grounded = c.state.on_ground;
            bool now_grounded = false;

            if (grounded_raw) {
                const float n_dot_up = hit.normal.y;
                if (n_dot_up >= slope_cos) {
                    // Snap to ground — treat impact as fully inelastic (only when falling).
                    if (v.y <= 0.0f) {
                        const float snap_y = hit.point_ws.y + c.desc.capsule_half_height_m + c.desc.capsule_radius_m;
                        c.state.position_ws.y = static_cast<double>(snap_y);
                        v.y = 0.0f;
                        now_grounded = true;
                    }
                    c.state.ground_normal    = hit.normal;
                    c.state.ground_surface_id = hit.surface_id;
                } else {
                    // Too steep — slide down the slope.
                    const glm::vec3 slope_dir = glm::normalize(glm::vec3{hit.normal.x, 0.0f, hit.normal.z});
                    v += slope_dir * 0.5f * dt;
                }
            }

            c.state.velocity = v;
            c.state.on_ground = now_grounded;
            if (now_grounded) {
                c.time_since_grounded_s = 0.0f;
                c.coyote_time_s = c.desc.coyote_time_s;
                c.state.in_coyote_window = false;
                if (!was_grounded) {
                    events::CharacterGrounded ev{c.desc.entity, c.state.ground_normal, c.state.ground_surface_id};
                    bus_char_grounded.publish(ev);
                }
            } else {
                c.time_since_grounded_s += dt;
                c.coyote_time_s = std::max(0.0f, c.coyote_time_s - dt);
                c.state.in_coyote_window = c.coyote_time_s > 0.0f;
            }

            c.pending_input.jump_pressed = false; // edge-triggered
        }
    }

    // ---- Query implementations ----
    bool raycast_closest_impl(const RaycastInput& in, const QueryFilter& filter,
                              RaycastHit& out, bool exclude_trigger) const {
        float best_t = in.max_distance_m;
        bool  any    = false;
        const glm::vec3 dir = glm::normalize(in.direction_ws);

        for (const auto& b : bodies) {
            if (!b.alive) continue;
            const LayerMask bit = layer_bit(b.layer);
            if ((filter.layer_mask & bit) == 0) continue;
            if (filter.exclude.id == b.id) continue;
            if (exclude_trigger && (b.layer == ObjectLayer::Trigger || b.is_sensor)) continue;
            if (filter.predicate && !filter.predicate(b.entity)) continue;

            // First try triangle-mesh test for static shapes (most accurate for slope tests).
            const NullShape* s = shape_ptr(b.shape);
            if (s && std::holds_alternative<TriangleMeshShapeDesc>(s->desc) && !s->tri_vertices.empty()) {
                const glm::vec3 offset{
                    static_cast<float>(b.state.position_ws.x),
                    static_cast<float>(b.state.position_ws.y),
                    static_cast<float>(b.state.position_ws.z)};
                for (std::size_t ti = 0; ti + 2 < s->tri_indices.size(); ti += 3) {
                    const glm::vec3 v0 = s->tri_vertices[s->tri_indices[ti + 0]] + offset;
                    const glm::vec3 v1 = s->tri_vertices[s->tri_indices[ti + 1]] + offset;
                    const glm::vec3 v2 = s->tri_vertices[s->tri_indices[ti + 2]] + offset;
                    float t{}; glm::vec3 n{};
                    if (ray_vs_triangle(in.origin_ws, dir, v0, v1, v2, best_t, t, n)) {
                        if (glm::dot(n, dir) > 0.0f) n = -n;
                        best_t = t;
                        out.body = BodyHandle{b.id};
                        out.entity = b.entity;
                        out.point_ws = in.origin_ws + dir * t;
                        out.normal = n;
                        out.fraction = t / std::max(1e-8f, in.max_distance_m);
                        out.surface_id = 0;
                        any = true;
                    }
                }
                continue;
            }

            glm::vec3 lo, hi;
            body_aabb(b, lo, hi);
            float t{}; glm::vec3 n{};
            if (ray_vs_aabb(in.origin_ws, dir, lo, hi, best_t, t, n)) {
                best_t = t;
                out.body = BodyHandle{b.id};
                out.entity = b.entity;
                out.point_ws = in.origin_ws + dir * t;
                out.normal = n;
                out.fraction = t / std::max(1e-8f, in.max_distance_m);
                out.surface_id = 0;
                any = true;
            }
        }
        return any;
    }

    bool raycast_any_impl(const RaycastInput& in, const QueryFilter& filter,
                          RaycastHit& out) const {
        // Null backend implements "any" as "closest" (still deterministic).
        return raycast_closest_impl(in, filter, out, /*exclude_trigger=*/false);
    }

    // ---- Debug draw ----
    void fill_debug_draw_impl(DebugDrawSink& sink) const {
        const std::uint32_t mask = sink.flag_mask() ? sink.flag_mask() : debug_flag_mask;
        if (mask == 0) return;

        auto push_aabb = [&](const glm::vec3& lo, const glm::vec3& hi, const glm::vec4& c) {
            if (!sink.can_push_line()) return;
            const glm::vec3 v[8]{
                {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
                {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z},
            };
            const int edges[12][2]{
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7},
            };
            for (const auto& e : edges) sink.push_line({v[e[0]], v[e[1]], c});
        };

        const bool show_bodies      = (mask & debug_flag_bit(DebugDrawFlag::Bodies))      != 0;
        const bool show_character   = (mask & debug_flag_bit(DebugDrawFlag::Character))   != 0;
        const bool show_constraints = (mask & debug_flag_bit(DebugDrawFlag::Constraints)) != 0;

        // WCAG AA palette — matches docs/a11y_phase12_selfcheck.md.
        const glm::vec4 c_static  {0.90f, 0.90f, 0.20f, 1.0f};
        const glm::vec4 c_dynamic {0.40f, 0.90f, 0.40f, 1.0f};
        const glm::vec4 c_sleep   {0.55f, 0.55f, 0.55f, 1.0f};
        const glm::vec4 c_trigger {0.30f, 0.70f, 0.95f, 1.0f};
        const glm::vec4 c_char    {0.95f, 0.75f, 0.20f, 1.0f};
        const glm::vec4 c_constr  {0.95f, 0.40f, 0.65f, 1.0f};

        if (show_bodies) {
            for (const auto& b : bodies) {
                if (!b.alive) continue;
                glm::vec3 lo, hi;
                body_aabb(b, lo, hi);
                glm::vec4 c = c_dynamic;
                if (b.state.motion_type == BodyMotionType::Static) c = c_static;
                if (!b.state.awake) c = c_sleep;
                if (b.layer == ObjectLayer::Trigger || b.is_sensor) c = c_trigger;
                push_aabb(lo, hi, c);
            }
        }

        if (show_character) {
            for (const auto& ch : characters) {
                if (!ch.alive) continue;
                const glm::vec3 pos{
                    static_cast<float>(ch.state.position_ws.x),
                    static_cast<float>(ch.state.position_ws.y),
                    static_cast<float>(ch.state.position_ws.z)};
                const glm::vec3 half{ch.desc.capsule_radius_m,
                                     ch.desc.capsule_half_height_m + ch.desc.capsule_radius_m,
                                     ch.desc.capsule_radius_m};
                push_aabb(pos - half, pos + half, c_char);
            }
        }

        if (show_constraints) {
            for (const auto& c : constraints) {
                if (!c.alive || c.broken) continue;
                const NullBody* ba = body_ptr(c.a);
                const NullBody* bb = body_ptr(c.b);
                if (!ba || !bb) continue;
                const glm::vec3 pa{static_cast<float>(ba->state.position_ws.x),
                                   static_cast<float>(ba->state.position_ws.y),
                                   static_cast<float>(ba->state.position_ws.z)};
                const glm::vec3 pb{static_cast<float>(bb->state.position_ws.x),
                                   static_cast<float>(bb->state.position_ws.y),
                                   static_cast<float>(bb->state.position_ws.z)};
                if (sink.can_push_line()) sink.push_line({pa, pb, c_constr});
            }
        }
    }

    // ---- hash ----
    void collect_entries(std::vector<HashBodyEntry>& out) const {
        out.clear();
        out.reserve(bodies.size());
        for (const auto& b : bodies) {
            if (!b.alive) continue;
            out.push_back(HashBodyEntry{b.id, b.state});
        }
        std::sort(out.begin(), out.end(),
                  [](const HashBodyEntry& a, const HashBodyEntry& b) {
                      return a.body_id < b.body_id;
                  });
    }
};

// ---------------------------------------------------------------------------
// PhysicsWorld public API — thin dispatchers.
// ---------------------------------------------------------------------------
PhysicsWorld::PhysicsWorld() : impl_(std::make_unique<Impl>()) {}
PhysicsWorld::~PhysicsWorld() = default;

bool PhysicsWorld::initialize(const PhysicsConfig& cfg,
                              events::CrossSubsystemBus* bus,
                              jobs::Scheduler* scheduler) {
    if (impl_->initialized_flag) return true;
    impl_->cfg = cfg;
    impl_->cross_bus = bus;
    impl_->scheduler = scheduler;
    impl_->initialized_flag = true;
    impl_->frame_index = 0;
    impl_->debug_flag_mask = 0;
    return true;
}

void PhysicsWorld::shutdown() {
    if (!impl_->initialized_flag) return;
    impl_->shapes.assign(1, NullShape{});
    impl_->bodies.assign(1, NullBody{});
    impl_->constraints.assign(1, NullConstraint{});
    impl_->characters.assign(1, NullCharacter{});
    impl_->vehicles.assign(1, NullVehicle{});
    impl_->free_bodies.clear();
    impl_->free_shapes.clear();
    impl_->free_constraints.clear();
    impl_->free_characters.clear();
    impl_->free_vehicles.clear();
    impl_->last_contacts.clear();
    impl_->time_accumulator_s = 0.0f;
    impl_->frame_index = 0;
    impl_->paused_flag = false;
    impl_->initialized_flag = false;
}

bool PhysicsWorld::initialized() const noexcept { return impl_->initialized_flag; }
BackendKind PhysicsWorld::backend() const noexcept { return BackendKind::Null; }
std::string_view PhysicsWorld::backend_name() const noexcept { return "null"; }
const PhysicsConfig& PhysicsWorld::config() const noexcept { return impl_->cfg; }

void PhysicsWorld::set_gravity(const glm::vec3& g) noexcept { impl_->cfg.gravity = g; }
glm::vec3 PhysicsWorld::gravity() const noexcept { return impl_->cfg.gravity; }

// ---- Shapes ----
ShapeHandle PhysicsWorld::create_shape(const ShapeDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_shapes.empty()) {
        id = impl_->free_shapes.back();
        impl_->free_shapes.pop_back();
        impl_->shapes[id] = NullShape{};
    } else {
        id = static_cast<std::uint32_t>(impl_->shapes.size());
        impl_->shapes.push_back(NullShape{});
    }
    auto& s = impl_->shapes[id];
    s.desc = desc;
    if (auto* mesh = std::get_if<TriangleMeshShapeDesc>(&desc)) {
        s.tri_vertices.assign(mesh->vertices.begin(), mesh->vertices.end());
        s.tri_indices.assign(mesh->indices.begin(), mesh->indices.end());
    }
    glm::vec3 centre{0.0f};
    const glm::vec3 half = shape_half_extents_local(desc, centre);
    s.local_aabb_min = centre - half;
    s.local_aabb_max = centre + half;
    return ShapeHandle{id};
}

void PhysicsWorld::destroy_shape(ShapeHandle shape) noexcept {
    if (!shape.valid() || shape.id >= impl_->shapes.size()) return;
    impl_->shapes[shape.id] = NullShape{};
    impl_->free_shapes.push_back(shape.id);
}

// ---- Bodies ----
BodyHandle PhysicsWorld::create_body(const RigidBodyDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_bodies.empty()) {
        id = impl_->free_bodies.back();
        impl_->free_bodies.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->bodies.size());
        impl_->bodies.push_back(NullBody{});
    }
    auto& b = impl_->bodies[id];
    b = NullBody{};
    b.alive = true;
    b.id = id;
    b.shape = desc.shape;
    b.layer = desc.layer;
    b.entity = desc.entity;
    b.mass_kg = desc.mass_kg;
    b.friction = desc.friction;
    b.restitution = desc.restitution;
    b.linear_damping = desc.linear_damping;
    b.angular_damping = desc.angular_damping;
    b.gravity_factor = desc.gravity_factor;
    b.is_sensor = desc.is_sensor;
    b.allow_sleeping = desc.allow_sleeping;
    b.continuous_collision = desc.continuous_collision;
    b.state.position_ws = desc.position_ws;
    b.state.rotation_ws = desc.rotation_ws;
    b.state.linear_velocity = desc.linear_velocity;
    b.state.angular_velocity = desc.angular_velocity;
    b.state.motion_type = desc.motion_type;
    b.state.awake = desc.motion_type == BodyMotionType::Dynamic;
    ++b.generation;
    return BodyHandle{id};
}

void PhysicsWorld::destroy_body(BodyHandle body) noexcept {
    auto* b = impl_->body_ptr(body);
    if (!b) return;
    b->alive = false;
    impl_->free_bodies.push_back(body.id);
}

void PhysicsWorld::set_body_transform(BodyHandle body, const glm::dvec3& pos, const glm::quat& rot) noexcept {
    if (auto* b = impl_->body_ptr(body)) {
        b->state.position_ws = pos;
        b->state.rotation_ws = rot;
    }
}

void PhysicsWorld::set_body_linear_velocity(BodyHandle body, const glm::vec3& v) noexcept {
    if (auto* b = impl_->body_ptr(body)) { b->state.linear_velocity = v; b->state.awake = true; b->idle_time = 0.0f; }
}

void PhysicsWorld::set_body_angular_velocity(BodyHandle body, const glm::vec3& v) noexcept {
    if (auto* b = impl_->body_ptr(body)) { b->state.angular_velocity = v; b->state.awake = true; b->idle_time = 0.0f; }
}

void PhysicsWorld::apply_force(BodyHandle body, const glm::vec3& force_n) noexcept {
    if (auto* b = impl_->body_ptr(body)) { b->accumulated_force += force_n; b->state.awake = true; b->idle_time = 0.0f; }
}

void PhysicsWorld::apply_impulse(BodyHandle body, const glm::vec3& impulse_ns) noexcept {
    if (auto* b = impl_->body_ptr(body)) { b->accumulated_impulse += impulse_ns; b->state.awake = true; b->idle_time = 0.0f; }
}

void PhysicsWorld::wake_body(BodyHandle body) noexcept {
    if (auto* b = impl_->body_ptr(body)) { b->state.awake = true; b->idle_time = 0.0f; }
}

void PhysicsWorld::put_to_sleep(BodyHandle body) noexcept {
    if (auto* b = impl_->body_ptr(body)) { b->state.awake = false; }
}

BodyState PhysicsWorld::body_state(BodyHandle body) const noexcept {
    if (const auto* b = impl_->body_ptr(body)) return b->state;
    return BodyState{};
}

std::size_t PhysicsWorld::body_count() const noexcept {
    std::size_t n = 0;
    for (const auto& b : impl_->bodies) if (b.alive) ++n;
    return n;
}

std::size_t PhysicsWorld::active_body_count() const noexcept {
    std::size_t n = 0;
    for (const auto& b : impl_->bodies) if (b.alive && b.state.awake) ++n;
    return n;
}

// ---- Constraints ----
ConstraintHandle PhysicsWorld::create_constraint(const ConstraintDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_constraints.empty()) {
        id = impl_->free_constraints.back();
        impl_->free_constraints.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->constraints.size());
        impl_->constraints.push_back(NullConstraint{});
    }
    auto& c = impl_->constraints[id];
    c = NullConstraint{};
    c.alive = true;
    c.desc = desc;
    auto fill = [&](BodyHandle ah, BodyHandle bh, float brk) {
        c.a = ah; c.b = bh; c.break_impulse = brk;
        if (const auto* ba = impl_->body_ptr(ah)) c.entity_a = ba->entity;
        if (const auto* bb = impl_->body_ptr(bh)) c.entity_b = bb->entity;
    };
    std::visit([&](const auto& d) {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, FixedConstraintDesc>)      fill(d.a, d.b, 0.0f);
        else if constexpr (std::is_same_v<T, DistanceConstraintDesc>) fill(d.a, d.b, d.break_impulse);
        else if constexpr (std::is_same_v<T, HingeConstraintDesc>)    fill(d.a, d.b, d.break_impulse);
        else if constexpr (std::is_same_v<T, SliderConstraintDesc>)   fill(d.a, d.b, d.break_impulse);
        else if constexpr (std::is_same_v<T, ConeConstraintDesc>)     fill(d.a, d.b, d.break_impulse);
        else if constexpr (std::is_same_v<T, SixDOFConstraintDesc>)   fill(d.a, d.b, d.break_impulse);
    }, desc);
    return ConstraintHandle{id};
}

void PhysicsWorld::destroy_constraint(ConstraintHandle ch) noexcept {
    if (!ch.valid() || ch.id >= impl_->constraints.size()) return;
    auto& c = impl_->constraints[ch.id];
    if (!c.alive) return;
    c.alive = false;
    impl_->free_constraints.push_back(ch.id);
}

// ---- Vehicles (null backend: no integration, surface only) ----
VehicleHandle PhysicsWorld::create_vehicle(const VehicleDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_vehicles.empty()) {
        id = impl_->free_vehicles.back();
        impl_->free_vehicles.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->vehicles.size());
        impl_->vehicles.push_back(NullVehicle{});
    }
    auto& v = impl_->vehicles[id];
    v = NullVehicle{};
    v.desc = desc;
    v.alive = true;
    return VehicleHandle{id};
}

void PhysicsWorld::destroy_vehicle(VehicleHandle vh) noexcept {
    if (!vh.valid() || vh.id >= impl_->vehicles.size()) return;
    auto& v = impl_->vehicles[vh.id];
    if (!v.alive) return;
    v.alive = false;
    impl_->free_vehicles.push_back(vh.id);
}

void PhysicsWorld::set_vehicle_input(VehicleHandle vh, const VehicleInput& input) noexcept {
    if (!vh.valid() || vh.id >= impl_->vehicles.size()) return;
    auto& v = impl_->vehicles[vh.id];
    if (!v.alive) return;
    v.input = input;
}

// ---- Characters ----
CharacterHandle PhysicsWorld::create_character(const CharacterDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_characters.empty()) {
        id = impl_->free_characters.back();
        impl_->free_characters.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->characters.size());
        impl_->characters.push_back(NullCharacter{});
    }
    auto& c = impl_->characters[id];
    c = NullCharacter{};
    c.alive = true;
    c.desc = desc;
    c.state.position_ws = desc.position_ws;
    c.state.velocity = glm::vec3{0.0f};
    c.state.on_ground = false;
    c.state.ground_normal = glm::vec3{0.0f, 1.0f, 0.0f};
    ++c.generation;
    return CharacterHandle{id};
}

void PhysicsWorld::destroy_character(CharacterHandle ch) noexcept {
    if (!ch.valid() || ch.id >= impl_->characters.size()) return;
    auto& c = impl_->characters[ch.id];
    if (!c.alive) return;
    c.alive = false;
    impl_->free_characters.push_back(ch.id);
}

void PhysicsWorld::set_character_input(CharacterHandle ch, const CharacterInput& input) noexcept {
    if (!ch.valid() || ch.id >= impl_->characters.size()) return;
    auto& c = impl_->characters[ch.id];
    if (!c.alive) return;
    // Jump is edge-triggered: OR into pending so repeated calls in the same
    // frame don't silently drop a press.
    c.pending_input.move_velocity_ws = input.move_velocity_ws;
    c.pending_input.jump_pressed = c.pending_input.jump_pressed || input.jump_pressed;
    c.pending_input.crouch_held = input.crouch_held;
}

CharacterState PhysicsWorld::character_state(CharacterHandle ch) const noexcept {
    if (!ch.valid() || ch.id >= impl_->characters.size()) return {};
    const auto& c = impl_->characters[ch.id];
    if (!c.alive) return {};
    return c.state;
}

// ---- Simulation ----
std::uint32_t PhysicsWorld::step(float delta_time_s) {
    if (!impl_->initialized_flag || impl_->paused_flag) return 0;
    impl_->time_accumulator_s += delta_time_s;
    const float dt = impl_->cfg.fixed_dt;
    const std::uint32_t max_sub = impl_->cfg.max_substeps;
    std::uint32_t n = 0;
    while (impl_->time_accumulator_s >= dt && n < max_sub) {
        step_fixed();
        impl_->time_accumulator_s -= dt;
        ++n;
    }
    // Clamp leftover to avoid unbounded catch-up after a hitch.
    if (impl_->time_accumulator_s > dt * static_cast<float>(max_sub)) {
        impl_->time_accumulator_s = dt;
    }
    return n;
}

void PhysicsWorld::step_fixed() {
    if (!impl_->initialized_flag) return;
    const float dt = impl_->cfg.fixed_dt;
    impl_->step_integrate(dt);
    impl_->step_resolve_collisions(dt);
    impl_->step_constraints(dt);
    impl_->step_characters(dt);
    ++impl_->frame_index;
}

float PhysicsWorld::interpolation_alpha() const noexcept {
    if (!impl_->initialized_flag) return 0.0f;
    const float dt = impl_->cfg.fixed_dt;
    return std::clamp(impl_->time_accumulator_s / dt, 0.0f, 1.0f);
}

void PhysicsWorld::reset() {
    if (!impl_->initialized_flag) return;
    impl_->shapes.assign(1, NullShape{});
    impl_->bodies.assign(1, NullBody{});
    impl_->constraints.assign(1, NullConstraint{});
    impl_->characters.assign(1, NullCharacter{});
    impl_->vehicles.assign(1, NullVehicle{});
    impl_->free_bodies.clear();
    impl_->free_shapes.clear();
    impl_->free_constraints.clear();
    impl_->free_characters.clear();
    impl_->free_vehicles.clear();
    impl_->last_contacts.clear();
    impl_->time_accumulator_s = 0.0f;
    impl_->frame_index = 0;
}

void PhysicsWorld::pause(bool paused) noexcept { impl_->paused_flag = paused; }
bool PhysicsWorld::paused() const noexcept     { return impl_->paused_flag; }
std::uint64_t PhysicsWorld::frame_index() const noexcept { return impl_->frame_index; }

// ---- Queries ----
bool PhysicsWorld::raycast_closest(const RaycastInput& in,
                                   const QueryFilter& filter,
                                   RaycastHit& out) const {
    return impl_->raycast_closest_impl(in, filter, out, /*exclude_trigger=*/false);
}

bool PhysicsWorld::raycast_any(const RaycastInput& in,
                               const QueryFilter& filter,
                               RaycastHit& out) const {
    return impl_->raycast_any_impl(in, filter, out);
}

void PhysicsWorld::raycast_batch(const RaycastBatch& batch, const QueryFilter& filter) const {
    const std::size_t n = batch.inputs.size();
    auto run = [&](std::size_t i) {
        RaycastHit hit{};
        const bool ok = impl_->raycast_closest_impl(batch.inputs[i], filter, hit, /*exclude_trigger=*/false);
        batch.hits[i] = hit;
        batch.hit_flags[i] = ok ? 1u : 0u;
    };
    if (impl_->scheduler && n > 32) {
        impl_->scheduler->parallel_for(static_cast<std::uint32_t>(n),
            [&](std::uint32_t idx) { run(static_cast<std::size_t>(idx)); });
    } else {
        for (std::size_t i = 0; i < n; ++i) run(i);
    }
}

bool PhysicsWorld::shape_sweep_closest(const ShapeSweepInput& in,
                                       const QueryFilter& filter,
                                       ShapeQueryHit& out) const {
    // Null backend approximation: treat as a ray from origin.
    RaycastInput rin{in.origin_ws, in.direction_ws, in.max_distance_m};
    RaycastHit rh{};
    if (!impl_->raycast_closest_impl(rin, filter, rh, /*exclude_trigger=*/false)) return false;
    out.body = rh.body;
    out.entity = rh.entity;
    out.point_ws = rh.point_ws;
    out.normal = rh.normal;
    out.penetration_depth = 0.0f;
    return true;
}

std::size_t PhysicsWorld::overlap(const OverlapInput& in,
                                  const QueryFilter& filter,
                                  std::span<ShapeQueryHit> out) const {
    const NullShape* s = impl_->shape_ptr(in.shape);
    if (!s) return 0;
    glm::vec3 centre{0.0f};
    const glm::vec3 half = shape_half_extents_local(s->desc, centre);
    const glm::vec3 lo = in.position_ws + centre - half;
    const glm::vec3 hi = in.position_ws + centre + half;

    std::size_t n = 0;
    for (const auto& b : impl_->bodies) {
        if (!b.alive) continue;
        const LayerMask bit = layer_bit(b.layer);
        if ((filter.layer_mask & bit) == 0) continue;
        if (filter.exclude.id == b.id) continue;
        if (filter.predicate && !filter.predicate(b.entity)) continue;
        glm::vec3 bl, bh;
        impl_->body_aabb(b, bl, bh);
        if (!aabb_vs_aabb(lo, hi, bl, bh)) continue;
        if (n < out.size()) {
            out[n] = ShapeQueryHit{};
            out[n].body = BodyHandle{b.id};
            out[n].entity = b.entity;
            out[n].point_ws = 0.5f * (glm::max(lo, bl) + glm::min(hi, bh));
            out[n].normal = glm::vec3{0.0f, 1.0f, 0.0f};
            out[n].penetration_depth = 0.0f;
        }
        ++n;
    }
    return n;
}

// ---- Determinism ----
std::uint64_t PhysicsWorld::determinism_hash(const HashOptions& opt) const {
    std::vector<HashBodyEntry> entries;
    impl_->collect_entries(entries);
    return gw::physics::determinism_hash(entries, opt);
}

void PhysicsWorld::collect_hash_entries(std::vector<HashBodyEntry>& out) const {
    impl_->collect_entries(out);
}

// ---- Debug draw ----
void PhysicsWorld::set_debug_flag_mask(std::uint32_t mask) noexcept { impl_->debug_flag_mask = mask; }
std::uint32_t PhysicsWorld::debug_flag_mask() const noexcept { return impl_->debug_flag_mask; }
void PhysicsWorld::fill_debug_draw(DebugDrawSink& sink) const {
    sink.set_max_lines(impl_->cfg.debug_max_lines);
    if (sink.flag_mask() == 0) sink.set_flag_mask(impl_->debug_flag_mask);
    impl_->fill_debug_draw_impl(sink);
}

events::EventBus<events::PhysicsContactAdded>&     PhysicsWorld::bus_contact_added()      noexcept { return impl_->bus_contact_added; }
events::EventBus<events::PhysicsContactPersisted>& PhysicsWorld::bus_contact_persisted()  noexcept { return impl_->bus_contact_persisted; }
events::EventBus<events::PhysicsContactRemoved>&   PhysicsWorld::bus_contact_removed()    noexcept { return impl_->bus_contact_removed; }
events::EventBus<events::TriggerEntered>&          PhysicsWorld::bus_trigger_entered()    noexcept { return impl_->bus_trigger_entered; }
events::EventBus<events::TriggerExited>&           PhysicsWorld::bus_trigger_exited()     noexcept { return impl_->bus_trigger_exited; }
events::EventBus<events::CharacterGrounded>&       PhysicsWorld::bus_character_grounded() noexcept { return impl_->bus_char_grounded; }
events::EventBus<events::ConstraintBroken>&        PhysicsWorld::bus_constraint_broken()  noexcept { return impl_->bus_constraint_broken; }

} // namespace gw::physics
