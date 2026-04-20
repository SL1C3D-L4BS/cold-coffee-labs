#pragma once

#include "vec.hpp"
#include <algorithm>

namespace gw {
namespace math {

template<typename T>
class Quat {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    constexpr Quat() noexcept : w_(T{1}), x_(T{}), y_(T{}), z_(T{}) {}
    constexpr Quat(T w, T x, T y, T z) noexcept 
        : w_(w), x_(x), y_(y), z_(z) {}
    
    [[nodiscard]] T w() const noexcept { return w_; }
    [[nodiscard]] T x() const noexcept { return x_; }
    [[nodiscard]] T y() const noexcept { return y_; }
    [[nodiscard]] T z() const noexcept { return z_; }
    
    // Quaternion multiplication
    [[nodiscard]] Quat operator*(const Quat& other) const noexcept {
        return Quat(
            w_ * other.w_ - x_ * other.x_ - y_ * other.y_ - z_ * other.z_,
            w_ * other.x_ + x_ * other.w_ + y_ * other.y_ + z_ * other.z_,
            z_ * other.x_ + x_ * other.z_ - y_ * other.w_ + w_ * other.y_,
            y_ * other.z_ - z_ * other.y_ + x_ * other.w_ - w_ * other.z_
        );
    }
    
    // Quaternion conjugate
    [[nodiscard]] Quat conjugate() const noexcept {
        return Quat(w_, -x_, -y_, -z_);
    }
    
    // Rotate vector by quaternion
    [[nodiscard]] Vec3<T> operator*(const Vec3<T>& v) const noexcept {
        // Using quaternion rotation formula: v' = q * v * q^-1
        // For unit quaternion q = [w, (x, y, z)], the rotation is:
        // v' = v + 2 * cross(q, v) * w
        const Quat qv(w_, x_, y_, z_);
        const auto cross_qv = cross(qv, v);
        const auto two_w = w_ + w_;
        return v + two_w * cross_qv;
    }
    
    // Normalize quaternion
    [[nodiscard]] Quat normalize() const noexcept {
        T len = std::sqrt(w_ * w_ + x_ * x_ + y_ * y_ + z_ * z_);
        if (len > T{0}) {
            return Quat(w_ / len, x_ / len, y_ / len, z_ / len);
        }
        return Quat(T{1}, T{}, T{}, T{});
    }
    
    // Spherical linear interpolation between two quaternions
    [[nodiscard]] static Quat slerp(const Quat& a, const Quat& b, T t) noexcept {
        // Calculate cosine of angle between quaternions
        T dot = a.w_ * b.w_ + a.x_ * b.x_ + a.y_ * b.y_ + a.z_ * b.z_;
        T angle = std::acos(std::clamp(dot, T{-1}, T{1}));
        
        // If angle is very small, use linear interpolation
        if (angle < T{0.0001}) {
            return a + (b - a) * t;
        }
        
        // Calculate interpolation factors
        T factor1 = std::sin((T{1} - t) * angle) / std::sin(angle);
        T factor2 = std::sin(t * angle) / std::sin(angle);
        
        return a * factor1 + b * factor2;
    }
    
private:
    T w_{};
    T x_{};
    T y_{};
    T z_{};
};

using Quatf = Quat<float>;
using Quatd = Quat<double>;

}  // namespace math
}  // namespace gw
