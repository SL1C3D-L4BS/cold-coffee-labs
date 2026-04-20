#include "math.hpp"

namespace gw {
namespace math {

// Standard library implementations for now
// SIMD optimizations can be added later
float sin(float angle) {
    return std::sin(angle);
}

float cos(float angle) {
    return std::cos(angle);
}

float sqrt(float x) {
    return std::sqrt(x);
}

}  // namespace math
}  // namespace gw
