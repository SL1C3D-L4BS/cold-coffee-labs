#pragma once
// engine/scene/seq/seq_cut.hpp
// Cut list + camera blend singleton — Phase 18-B.
//
// ADR-0086: CutListComponent uses static capacity of 64 entries (not dynamic).
// Rationale: cinematics rarely exceed 20 cuts; static array avoids per-frame heap pressure
// in SequencerSystem::tick which runs every frame. If a sequence needs >64 cuts, split
// it into multiple .gwseq assets and chain them via SeqEventTrackComponent callbacks.

#include "engine/ecs/entity.hpp"
#include "engine/ecs/world.hpp"

#include <array>
#include <cstdint>

namespace gw::seq {

/// Easing for blend progress in [0,1].
enum class CutBlendCurve : std::uint8_t {
    Cut     = 0,
    Linear  = 1,
    EaseOut = 2,
};

/// One camera transition.
struct CutEntry {
    gw::ecs::Entity from_camera{};
    gw::ecs::Entity to_camera{};
    std::uint32_t   frame         = 0;
    std::uint32_t   blend_frames  = 0;
    CutBlendCurve   blend_curve   = CutBlendCurve::Cut;
};

/// Authoring list of cuts (lives on director / player entity).
struct CutListComponent {
    std::array<CutEntry, 64> cuts{};
    std::uint32_t            cut_count = 0;
};

/// Global blend state written by `CutSystem` and consumed by
/// `CinematicCameraSystem` for two-camera interpolation.
struct CameraBlendStateComponent {
    gw::ecs::Entity cam_a{};
    gw::ecs::Entity cam_b{};
    float           weight         = 0.f;
    bool            blend_active = false;
};

struct CutSystem {
    static void tick(gw::ecs::World& world, double play_head_frame) noexcept;
};

}  // namespace gw::seq
