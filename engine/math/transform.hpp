#pragma once

#include "vec.hpp"
#include "quat.hpp"
#include "mat.hpp"

namespace gw {
namespace math {

template<typename T>
class Transform {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    constexpr Transform() noexcept 
        : position_(T{}), rotation_(T{1}, T{}, T{}), scale_(T{1}) {}
    
    constexpr Transform(const Vec3<T>& position, const Quat<T>& rotation, const Vec3<T>& scale = T{1}) noexcept
        : position_(position), rotation_(rotation), scale_(scale) {}
    
    // Accessors
    [[nodiscard]] const Vec3<T>& position() const noexcept { return position_; }
    [[nodiscard]] const Quat<T>& rotation() const noexcept { return rotation_; }
    [[nodiscard]] const Vec3<T>& scale() const noexcept { return scale_; }
    
    // Operations
    [[nodiscard]] Vec3<T> transform_point(const Vec3<T>& point) const noexcept {
        return position_ + (rotation_ * (point - position_) * scale_);
    }
    
    [[nodiscard]] Vec3<T> transform_direction(const Vec3<T>& dir) const noexcept {
        return position_ + (rotation_ * dir) * scale_;
    }
    
    // Matrix representation (for shader uniforms)
    [[nodiscard]] Mat3<T> matrix() const noexcept {
        // Create rotation matrix from quaternion and scale
        const auto rot_mat = rotation_.matrix();
        const auto scale_mat = Mat3<T>::scale(scale_);
        
        // Combine: scale * rotation * translation
        // Note: translation is implicit (position is origin + translation)
        return scale_mat * rot_mat;
    }
    
    [[nodiscard]] Mat3<T> matrix_no_translation() const noexcept {
        // Rotation matrix without translation (for pure rotations)
        return rotation_.matrix();
    }
    
private:
    Vec3<T> position_;
    Quat<T> rotation_;
    Vec3<T> scale_;
};

// World-space transforms MUST be `Transform3d` (f64) per
// docs/01_CONSTITUTION_AND_PROGRAM.md §2.6 and CLAUDE.md NN #17.
// `Transform<float>` is intentionally NOT aliased here: local-to-chunk math
// and GPU-bound data should use explicit `Vec3f` / `glm::vec3` at the call
// site so the precision demotion is visible in code review.
using Transform3d = Transform<double>;

}  // namespace math
}  // namespace gw
