#pragma once
// engine/render/hal/hdr_output.hpp — Part C §21 scaffold (ADR-0112b).
//
// HDR10 swapchain output via VK_EXT_hdr_metadata. Polaris supports HDR10;
// gated behind CVar `r.HDROutput`. SDR remains the default.

#include <cstdint>

namespace gw::render::hal {

struct Hdr10Metadata {
    float display_primaries_r[2] {0.680f, 0.320f};   // BT.2020 Red
    float display_primaries_g[2] {0.265f, 0.690f};   // BT.2020 Green
    float display_primaries_b[2] {0.150f, 0.060f};   // BT.2020 Blue
    float white_point[2]         {0.3127f, 0.3290f}; // D65
    float max_luminance_cd_m2    = 1000.f;
    float min_luminance_cd_m2    = 0.005f;
    float max_content_light_level_cd_m2 = 1000.f;
    float max_frame_average_light_level_cd_m2 = 400.f;
};

enum class OutputFormat : std::uint8_t {
    Sdr8bit   = 0,   // VK_FORMAT_B8G8R8A8_SRGB
    Hdr10pq   = 1,   // VK_FORMAT_A2B10G10R10_UNORM_PACK32 + PQ tonemap
    Hdr16fp   = 2,   // VK_FORMAT_R16G16B16A16_SFLOAT (scRGB)
};

/// Query whether the current swapchain / display supports HDR10.
[[nodiscard]] bool hdr10_supported() noexcept;

/// Apply HDR10 metadata to the active swapchain (VK_EXT_hdr_metadata).
/// Returns false if the display is SDR or the extension is not present.
bool apply_hdr10_metadata(const Hdr10Metadata& md) noexcept;

} // namespace gw::render::hal
