#pragma once
// engine/render/post/post_types.hpp — ADR-0079/0080 Wave 17F.

#include <array>
#include <cstdint>
#include <string>

namespace gw::render::post {

enum class TonemapCurve : std::uint8_t {
    Linear     = 0,
    PbrNeutral = 1,   // Khronos PBR Neutral — default (ADR-0080)
    ACES       = 2,
};

[[nodiscard]] TonemapCurve parse_tonemap_curve(const std::string&) noexcept;
[[nodiscard]] const char*  to_cstring(TonemapCurve) noexcept;

enum class TaaClipMode : std::uint8_t {
    Aabb     = 0,
    Variance = 1,
    KDop     = 2,      // ADR-0079 default
};

[[nodiscard]] TaaClipMode parse_clip_mode(const std::string&) noexcept;
[[nodiscard]] const char* to_cstring(TaaClipMode) noexcept;

struct PostStats {
    double   last_post_ms{0.0};
    double   bloom_ms{0.0};
    double   taa_ms{0.0};
    double   mb_ms{0.0};
    double   dof_ms{0.0};
    double   tonemap_ms{0.0};
    std::uint64_t frame{0};
};

} // namespace gw::render::post
