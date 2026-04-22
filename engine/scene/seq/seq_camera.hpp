#pragma once
// engine/scene/seq/seq_camera.hpp
// Cinematic camera selection + render view output — Phase 18-A.

#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"

#include <cstdint>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace gw::seq {

/// Authoring component: marks an entity as a cinematic camera candidate.
struct CinematicCameraComponent {
    bool         active      = false;
    float        fov_deg     = 60.f;
    float        near_plane  = 0.05f;
    float        far_plane   = 2000.f;
    std::int32_t priority    = 0;
};

/// Optional blend directive carried on the **winning** camera entity. While
/// `elapsed_frames < duration_frames`, the system linearly interpolates eye
/// position and lens parameters toward this entity's authored camera values,
/// starting from `partner`'s camera at the beginning of the blend.
struct CinematicCameraBlendComponent {
    gw::ecs::Entity partner{};
    std::uint32_t   duration_frames = 0;
    std::uint32_t   elapsed_frames  = 0;
};

/// Small render-facing snapshot produced by `CinematicCameraSystem::tick`.
struct CinematicRenderView {
    bool      valid     = false;
    glm::mat4 view{1.f};
    float     fov_y_deg = 60.f;
    float     near_z    = 0.05f;
    float     far_z     = 2000.f;
};

/// Selects the active cinematic camera and publishes a view matrix for the
/// renderer. Position is read from `gw::editor::scene::TransformComponent` on
/// the same entity (optionally driven upstream by `SequencerSystem` spline
/// tracks). Cut transitions are expressed by stepping `active`; blends use
/// `CinematicCameraBlendComponent`.
class CinematicCameraSystem {
public:
    CinematicCameraSystem() noexcept = default;

    /// Picks the highest-priority active camera, applies optional linear
    /// blending, and writes `output()`.
    void tick(gw::ecs::World& world) noexcept;

    /// Last published render view (valid flag false when no camera active).
    [[nodiscard]] const CinematicRenderView& output() const noexcept { return out_; }

private:
    CinematicRenderView out_{};
};

}  // namespace gw::seq
