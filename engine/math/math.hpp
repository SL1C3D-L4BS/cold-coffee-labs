#pragma once

#include "vec.hpp"
#include "mat.hpp"
#include "transform.hpp"

namespace gw {
namespace math {

// Mathematical constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;
constexpr float EPSILON = 1e-6f;

// Utility functions
template<typename T>
[[nodiscard]] constexpr T clamp(T value, T min_val, T max_val) noexcept {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

template<typename T>
[[nodiscard]] constexpr T lerp(T a, T b, T t) noexcept {
    return a + t * (b - a);
}

template<typename T>
[[nodiscard]] constexpr T smoothstep(T edge, T x) noexcept {
    return x <= edge ? T{0} : T{1};
}

// Angle and trigonometry functions
template<typename T>
[[nodiscard]] T sin(T angle) noexcept {
    return std::sin(angle);
}

template<typename T>
[[nodiscard]] T cos(T angle) noexcept {
    return std::cos(angle);
}

template<typename T>
[[nodiscard]] T abs(T value) noexcept {
    return value < T{0} ? -value : value;
}

template<typename T>
[[nodiscard]] T min(T a, T b) noexcept {
    return a < b ? a : b;
}

template<typename T>
[[nodiscard]] T max(T a, T b) noexcept {
    return a > b ? a : b;
}

// SIMD feature detection (simplified)
inline bool has_sse2() noexcept {
#ifdef _M_IX86
    return true;
#else
    return false;
#endif
}

inline bool has_avx2() noexcept {
#ifdef __AVX2__
    return true;
#else
    return false;
#endif
}

inline bool has_fma3() noexcept {
#ifdef __FMA__
    return true;
#else
    return false;
#endif
}

}  // namespace math
}  // namespace gw
