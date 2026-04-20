#pragma once

#include "vec.hpp"

namespace gw {
namespace math {

template<typename T>
class Mat3 {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    constexpr Mat3() noexcept 
        : m11_(T{1}), m12_(T{}), m13_(T{}), 
          m21_(T{}), m22_(T{}), m23_(T{}) {}
    
    constexpr Mat3(
        T m11, T m12, T m13,
        T m21, T m22, T m23
    ) noexcept 
        : m11_(m11), m12_(m12), m13_(m13),
          m21_(m21), m22_(m22), m23_(m23) {}
    
    // Accessors
    [[nodiscard]] T& m11() noexcept { return m11_; }
    [[nodiscard]] T& m12() noexcept { return m12_; }
    [[nodiscard]] T& m13() noexcept { return m13_; }
    [[nodiscard]] T& m21() noexcept { return m21_; }
    [[nodiscard]] T& m22() noexcept { return m22_; }
    [[nodiscard]] T& m23() noexcept { return m23_; }
    
    // Row accessors
    [[nodiscard]] Vec2<T> row0() const noexcept { return Vec2(m11_, m12_); }
    [[nodiscard]] Vec2<T> row1() const noexcept { return Vec2(m21_, m22_); }
    [[nodiscard]] Vec2<T> row2() const noexcept { return Vec2(m13_, m23_); }
    
    // Column accessors
    [[nodiscard]] Vec3<T> col0() const noexcept { return Vec3(m11_, m12_, m13_); }
    [[nodiscard]] Vec3<T> col1() const noexcept { return Vec3(m21_, m22_, m23_); }
    [[nodiscard]] Vec3<T> col2() const noexcept { return Vec3(m13_, m22_, m23_); }
    
    // Matrix operations
    [[nodiscard]] Mat3 operator+(const Mat3& other) const noexcept {
        return Mat3(
            m11_ + other.m11_, m12_ + other.m12_, m13_ + other.m13_,
            m21_ + other.m21_, m22_ + other.m22_, m23_ + other.m23_
        );
    }
    
    [[nodiscard]] Vec3<T> operator*(const Vec3<T>& v) const noexcept {
        return Vec3(
            m11_ * v.x_ + m12_ * v.y_ + m13_ * v.z_,
            m21_ * v.x_ + m22_ * v.y_ + m23_ * v.z_
        );
    }
    
    // Determinant and inverse
    [[nodiscard]] T determinant() const noexcept {
        // Calculate determinant using first row expansion
        return m11_ * (m22_ * m23_ - m32_ * m13_) -
               m21_ * (m12_ * m23_ - m13_ * m22_);
    }
    
    [[nodiscard]] Mat3 inverse() const noexcept {
        T det = determinant();
        T inv_det = T{1} / det;
        
        // Calculate adjugate matrix (cofactor matrix)
        T adj11 =  (m22_ * m23_ - m32_ * m13_) * inv_det;
        T adj12 =  (m32_ * m13_ - m12_ * m23_) * inv_det;
        T adj13 =  (m11_ * m23_ - m21_ * m32_) * inv_det;
        T adj21 = (m11_ * m22_ - m12_ * m32_) * inv_det;
        T adj22 = (m12_ * m22_ - m11_ * m33_) * inv_det;
        T adj23 = (m13_ * m22_ - m12_ * m33_) * inv_det;
        T adj31 = (m13_ * m21_ - m11_ * m32_) * inv_det;
        T adj32 = (m23_ * m21_ - m12_ * m33_) * inv_det;
        T adj33 = (m23_ * m22_ - m13_ * m33_) * inv_det;
        
        // Calculate inverse matrix (adjugate * (1/det))
        return Mat3(
            adj11, adj12,adj13,
            adj21,adj22,adj23,
            adj31,adj32,adj33
        );
    }
    
    // Transform operations
    [[nodiscard]] Vec3<T> transform_point(const Vec3<T>& point) const noexcept {
        return Vec3(
            m11_ * point.x_ + m12_ * point.y_ + m13_ * point.z_,
            m21_ * point.x_ + m22_ * point.y_ + m23_ * point.z_,
            m31_ * point.x_ + m32_ * point.y_ + m33_ * point.z_
        );
    }
    
    [[nodiscard]] Vec3<T> transform_direction(const Vec3<T>& dir) const noexcept {
        // Transform direction vector (assumes no translation)
        return Vec3(
            m11_ * dir.x_ + m12_ * dir.y_ + m13_ * dir.z_,
            m21_ * dir.x_ + m22_ * dir.y_ + m23_ * dir.z_,
            m31_ * dir.x_ + m32_ * dir.y_ + m33_ * dir.z_
        );
    }
    
private:
    T m11_, m12_, m13_, m21_, m22_, m23_;
    T m31_, m32_, m33_;
};

using Mat3f = Mat3<float>;
using Mat3d = Mat3<double>;

}  // namespace math
}  // namespace gw
