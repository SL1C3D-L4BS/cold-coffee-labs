#pragma once
// engine/render/post/fsr2/fsr2_integration.hpp — Part C §21 scaffold
// (ADR-0112). Gated behind GW_ENABLE_FSR2 so RX 580 can opt in to the
// quality / performance upscaler without pulling AMD's SDK into CI.

#include <cstdint>

namespace gw::render::post::fsr2 {

enum class UpscaleMode : std::uint8_t {
    Off             = 0,
    Quality         = 1,  // ~1.5x
    Balanced        = 2,  // ~1.7x
    Performance     = 3,  // ~2.0x
    UltraPerformance= 4,  // ~3.0x
};

struct Fsr2Params {
    UpscaleMode mode               = UpscaleMode::Off;
    float       sharpness_0_1      = 0.25f;
    float       motion_vector_scale = 1.f;
    bool        hdr                = false;
};

/// Result from a single-frame FSR 2 invocation. Scaffold-only; the real
/// integration binds AMD's Vulkan SDK behind `GW_ENABLE_FSR2=ON`.
struct Fsr2Dispatch {
    bool       ok               = false;
    float      upscale_ms       = 0.f;
    std::uint32_t render_w      = 0;
    std::uint32_t render_h      = 0;
    std::uint32_t display_w     = 0;
    std::uint32_t display_h     = 0;
};

Fsr2Dispatch dispatch(const Fsr2Params& params) noexcept;

/// No-Frame-Generation posture (explicit): FSR 3 Frame Gen is unviable on
/// Polaris and is rejected at compile time.
inline constexpr bool FRAME_GENERATION_ENABLED = false;

} // namespace gw::render::post::fsr2
