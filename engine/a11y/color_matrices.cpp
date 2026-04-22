// engine/a11y/color_matrices.cpp — ADR-0071 §3.

#include "engine/a11y/color_matrices.hpp"

#include <algorithm>

namespace gw::a11y {

ColorMode parse_color_mode(std::string_view s) noexcept {
    if (s == "none") return ColorMode::None;
    if (s == "protan" || s == "protanopia") return ColorMode::Protanopia;
    if (s == "deutan" || s == "deuteranopia") return ColorMode::Deuteranopia;
    if (s == "tritan" || s == "tritanopia") return ColorMode::Tritanopia;
    if (s == "hc" || s == "high_contrast" || s == "highcontrast") return ColorMode::HighContrast;
    return ColorMode::None;
}

namespace {
constexpr ColorMatrix3 kIdentity{{ 1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f }};
constexpr ColorMatrix3 kProtan  {{ 0.152f, 1.053f, -0.205f,
                                    0.115f, 0.786f,  0.099f,
                                   -0.004f,-0.048f,  1.052f }};
constexpr ColorMatrix3 kDeutan  {{ 0.367f, 0.861f, -0.228f,
                                    0.280f, 0.673f,  0.047f,
                                   -0.012f, 0.043f,  0.969f }};
constexpr ColorMatrix3 kTritan  {{ 1.256f,-0.077f, -0.179f,
                                    0.078f, 0.931f, -0.009f,
                                   -0.007f, 0.018f,  0.990f }};
constexpr ColorMatrix3 kHC      {{ 1.5f, 0.f, 0.f,
                                    0.f, 1.5f, 0.f,
                                    0.f, 0.f, 1.5f }};
} // namespace

ColorMatrix3 color_matrix_for(ColorMode m) noexcept {
    switch (m) {
        case ColorMode::None:         return kIdentity;
        case ColorMode::Protanopia:   return kProtan;
        case ColorMode::Deuteranopia: return kDeutan;
        case ColorMode::Tritanopia:   return kTritan;
        case ColorMode::HighContrast: return kHC;
    }
    return kIdentity;
}

ColorMatrix3 color_matrix_for(ColorMode m, float strength) noexcept {
    const float s = std::clamp(strength, 0.0f, 1.0f);
    if (s <= 0.0f) return kIdentity;
    const ColorMatrix3 target = color_matrix_for(m);
    if (s >= 1.0f) return target;
    ColorMatrix3 out{};
    for (std::size_t i = 0; i < 9; ++i) {
        out.m[i] = kIdentity.m[i] * (1.0f - s) + target.m[i] * s;
    }
    return out;
}

} // namespace gw::a11y
