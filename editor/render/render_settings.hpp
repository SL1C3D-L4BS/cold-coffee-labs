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
// LuminanceHistogram — 64-bucket luminance distribution drawn by the
// HistogramPanel. The renderer updates `buckets` each frame; until the
// reference-renderer histogram lands, the editor synthesises a plausible
// shape so the cockpit looks populated end-to-end.
// -----------------------------------------------------------------------
struct LuminanceHistogram {
    static constexpr std::size_t kBucketCount = 64;
    std::array<float, kBucketCount> buckets{};
};

// -----------------------------------------------------------------------
// LightingParams — ambient colour + a fixed-count of dynamic lights.
// The cockpit's "light" panel cycles through `lights[]` via an int
// index; `light_count` caps the iterated subset.
// -----------------------------------------------------------------------
struct DynamicLight {
    float color[3]    = {1.f, 1.f, 1.f};
    float intensity   = 1.0f;
    float position[3] = {0.f, 3.f, 0.f};
    float range       = 10.0f;
    bool  enabled     = true;
};

struct LightingParams {
    float ambient_color[3] = {0.56f, 0.65f, 0.90f};  // image's blue sky tint
    float ambient_intensity = 0.25f;
    int   active_light      = 0;                      // UI "light N" selector
    int   light_count       = 2;
    std::array<DynamicLight, 8> lights{};
};

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
    float gpu_ms           = 0.f;    // reserved for when the renderer fills it
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
};

}  // namespace gw::editor::render
