#pragma once
// editor/render/render_settings.hpp
// Shared, in-memory render-pipeline settings edited by the cockpit panels
// (stats / tone-map / SSAO / exposure / lighting) and consumed by the scene
// pass. Phase-7 Path-A gate E wires the *edit surface*; the scene renderer
// honours the params progressively (a clear colour today, forward+ later).
//
// This header is the single source of truth for every slider & toggle shown
// in the editor-right / settings panels. The panels hold a non-owning pointer
// into an EditorApplication-owned instance. Everything is trivially-copyable
// so PIE snapshots + `.gwscene` save can roll it in without ceremony once
// Phase-8 lands the scene-level settings record.

#include "engine/render/authoring_lighting.hpp"

#include <array>
#include <cstdint>

namespace gw::editor::render {

// -----------------------------------------------------------------------
// ToneMapParams — Uncharted-2/LottesStyle tone-curve parameters.
// The controls map 1:1 onto the image's "tone map options" panel.
// -----------------------------------------------------------------------
struct ToneMapParams {
    float max_brightness  = 1.00f;   // ceiling of the mapped range
    float contrast        = 1.80f;
    float linear_section  = 0.40f;   // width of the mid-tone linear band
    float black_lightness = 0.05f;   // pedestal lift (shadow toe)
    float pedestal        = 0.00f;   // additive black offset
    float gamma           = 2.20f;   // output gamma
};

// -----------------------------------------------------------------------
// SSAOParams — screen-space ambient occlusion sliders.
// -----------------------------------------------------------------------
struct SSAOParams {
    bool  enabled      = true;
    int   sample_count = 16;
    float radius       = 0.25f;
    float bias         = 0.025f;
    float power        = 2.00f;
};

// -----------------------------------------------------------------------
// ExposureParams — auto-exposure histogram bounds.
// -----------------------------------------------------------------------
struct ExposureParams {
    float min_log_luminance = -3.0f;
    float max_log_luminance =  1.0f;
    float gamma             =  2.20f;   // final colour-space gamma
    // Live readout (written by the renderer; displayed by the panel).
    float average_luminance =  0.369139f;
};

// -----------------------------------------------------------------------
// LuminanceHistogram — 64-bucket luminance distribution. The editor fills
// `buckets` from a small GPU readback of the viewport scene colour each frame.
// -----------------------------------------------------------------------
struct LuminanceHistogram {
    static constexpr std::size_t kBucketCount = 64;
    std::array<float, kBucketCount> buckets{};
};

using DynamicLight   = gw::render::authoring::DynamicLight;
using LightingParams = gw::render::authoring::LightingParams;

// -----------------------------------------------------------------------
// DebugToggles — viewport/debug-draw switches exposed in the stats panel.
// -----------------------------------------------------------------------
struct DebugToggles {
    bool draw_grid            = true;
    bool draw_wireframe       = false;
    bool draw_gizmos          = true;
    bool draw_debug_lines     = true;
    bool draw_bounds          = false;
    bool draw_light_volumes   = false;
    bool freeze_culling       = false;
};

// -----------------------------------------------------------------------
// FrameStats — sampled every frame by EditorApplication; rendered by the
// stats panel (FPS + frame-time chart).
// -----------------------------------------------------------------------
struct FrameStats {
    float fps              = 0.f;
    float cpu_ms           = 0.f;    // time this frame spent on the CPU
    /// Editor CB: time between the ALL_COMMANDS timestamp (after offscene RT+hist)
    /// and the end of the main swapchain ImGui pass — i.e. mostly the docking UI,
    /// not the viewport scene RT. Full GPU frame graph is post–Phase 8.
    float gpu_ms = 0.f;
    std::uint64_t frame_id = 0;

    static constexpr std::size_t kHistoryCount = 128;
    std::array<float, kHistoryCount> fps_history{};
    std::array<float, kHistoryCount> ms_history{};
    std::size_t history_head = 0;    // ring-buffer index

    void push_sample(float fps_now, float cpu_ms_now) noexcept {
        fps     = fps_now;
        cpu_ms  = cpu_ms_now;
        ++frame_id;
        fps_history[history_head] = fps_now;
        ms_history[history_head]  = cpu_ms_now;
        history_head = (history_head + 1) % kHistoryCount;
    }
};

// -----------------------------------------------------------------------
// RenderSettings — one-stop struct every cockpit panel binds to.
// Owned by EditorApplication; panels carry a raw pointer (lifetime is
// tied to the application and the panels are owned by the registry).
// -----------------------------------------------------------------------
struct RenderSettings {
    ToneMapParams      tonemap{};
    SSAOParams         ssao{};
    ExposureParams     exposure{};
    LuminanceHistogram histogram{};
    LightingParams     lighting{};
    DebugToggles       debug{};
    FrameStats         stats{};

    // pre-eng-fsr2-hal-frame-graph: cockpit toggles for the FSR 2 upscale pass
    // and HDR10 swapchain output. Scaffold calls live in
    // `engine/render/post/upscale_and_hdr_bridge.cpp`; real frame-graph pass
    // registration lands with the Phase 21 post-FX wave.
    bool fsr2_enabled = false;
    bool hdr_enabled  = false;
};

}  // namespace gw::editor::render
