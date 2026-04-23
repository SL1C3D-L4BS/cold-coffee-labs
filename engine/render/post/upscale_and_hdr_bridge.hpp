#pragma once
// engine/render/post/upscale_and_hdr_bridge.hpp — pre-eng-fsr2-hal-frame-graph
//
// Call-site bridge for the FSR 2 upscale pass + HDR10 swapchain output.
// Consumed by the frame-graph builder once it lands full-scope Phase 21
// post-FX registration. Today this exists so the scaffolded FSR 2 dispatch
// and HDR10 metadata apply functions are no longer orphan symbols reported
// by `docs/prompts/00_WIRE_UP_AUDIT.md`.

#include "engine/render/hal/hdr_output.hpp"
#include "engine/render/post/fsr2/fsr2_integration.hpp"

namespace gw::render::post {

struct UpscaleHdrConfig {
    bool fsr2_enabled = false;
    bool hdr_enabled  = false;
    fsr2::UpscaleMode       fsr2_mode = fsr2::UpscaleMode::Off;
    gw::render::hal::Hdr10Metadata hdr{};
};

struct UpscaleHdrResult {
    fsr2::Fsr2Dispatch fsr2_out{};
    bool               hdr_applied = false;
};

/// Execute the per-frame upscale + HDR application. No-op when both toggles
/// are off. Called from the frame-graph builder after scene shading and
/// before present.
UpscaleHdrResult apply_upscale_and_hdr(const UpscaleHdrConfig& cfg) noexcept;

} // namespace gw::render::post
