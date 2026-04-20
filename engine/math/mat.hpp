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
        return m11_ * (m22_ * m33_ - m32_ * m23_) -
               m12_ * (m21_ * m33_ - m31_ * m23_) +
               m13_ * (m21_ * m32_ - m31_ * m22_);
    }
    
    [[nodiscard]] Mat3 inverse() const noexcept {
        T det = determinant();
        if (det == T{0}) {
            return Mat3(); // Return identity if singular
        }
        T inv_det = T{1} / det;
        
        // Calculate adjugate matrix (cofactor matrix)
        T adj11 =  (m22_ * m33_ - m32_ * m23_) * inv_det;
        T adj12 = -(m12_ * m33_ - m32_ * m13_) * inv_det;
        T adj13 =  (m12_ * m23_ - m22_ * m13_) * inv_det;
        T adj21 = -(m21_ * m33_ - m31_ * m23_) * inv_det;
        T adj22 =  (m11_ * m33_ - m31_ * m13_) * inv_det;
        T adj23 = -(m11_ * m23_ - m21_ * m13_) * inv_det;
        T adj31 =  (m21_ * m32_ - m31_ * m22_) * inv_det;
        T adj32 = -(m11_ * m32_ - m31_ * m12_) * inv_det;
        T adj33 =  (m11_ * m22_ - m21_ * m12_) * inv_det;
        
        // Calculate inverse matrix (adjugate * (1/det))
        return Mat3(
            adj11, adj12, adj13,
            adj21, adj22, adj23,
            adj31, adj32, adj33
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

template<typename T>
class Mat4 {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    constexpr Mat4() noexcept 
        : m11_(T{1}), m12_(T{}), m13_(T{}), m14_(T{}),
          m21_(T{}), m22_(T{}), m23_(T{}), m24_(T{}),
          m31_(T{}), m32_(T{}), m33_(T{}), m34_(T{}),
          m41_(T{}), m42_(T{}), m43_(T{}), m44_(T{1}) {}
    
    constexpr Mat4(
        T m11, T m12, T m13, T m14,
        T m21, T m22, T m23, T m24,
        T m31, T m32, T m33, T m34,
        T m41, T m42, T m43, T m44
    ) noexcept 
        : m11_(m11), m12_(m12), m13_(m13), m14_(m14),
          m21_(m21), m22_(m22), m23_(m23), m24_(m24),
          m31_(m31), m32_(m32), m33_(m33), m34_(m34),
          m41_(m41), m42_(m42), m43_(m43), m44_(m44) {}
    
    // Accessors
    [[nodiscard]] T& m11() noexcept { return m11_; }
    [[nodiscard]] T& m12() noexcept { return m12_; }
    [[nodiscard]] T& m13() noexcept { return m13_; }
    [[nodiscard]] T& m14() noexcept { return m14_; }
    [[nodiscard]] T& m21() noexcept { return m21_; }
    [[nodiscard]] T& m22() noexcept { return m22_; }
    [[nodiscard]] T& m23() noexcept { return m23_; }
    [[nodiscard]] T& m24() noexcept { return m24_; }
    [[nodiscard]] T& m31() noexcept { return m31_; }
    [[nodiscard]] T& m32() noexcept { return m32_; }
    [[nodiscard]] T& m33() noexcept { return m33_; }
    [[nodiscard]] T& m34() noexcept { return m34_; }
    [[nodiscard]] T& m41() noexcept { return m41_; }
    [[nodiscard]] T& m42() noexcept { return m42_; }
    [[nodiscard]] T& m43() noexcept { return m43_; }
    [[nodiscard]] T& m44() noexcept { return m44_; }
    
    // Row accessors
    [[nodiscard]] Vec4<T> row0() const noexcept { return Vec4(m11_, m12_, m13_, m14_); }
    [[nodiscard]] Vec4<T> row1() const noexcept { return Vec4(m21_, m22_, m23_, m24_); }
    [[nodiscard]] Vec4<T> row2() const noexcept { return Vec4(m31_, m32_, m33_, m34_); }
    [[nodiscard]] Vec4<T> row3() const noexcept { return Vec4(m41_, m42_, m43_, m44_); }
    
    // Column accessors
    [[nodiscard]] Vec4<T> col0() const noexcept { return Vec4(m11_, m21_, m31_, m41_); }
    [[nodiscard]] Vec4<T> col1() const noexcept { return Vec4(m12_, m22_, m32_, m42_); }
    [[nodiscard]] Vec4<T> col2() const noexcept { return Vec4(m13_, m23_, m33_, m43_); }
    [[nodiscard]] Vec4<T> col3() const noexcept { return Vec4(m14_, m24_, m34_, m44_); }
    
    // Matrix operations
    [[nodiscard]] Mat4 operator+(const Mat4& other) const noexcept {
        return Mat4(
            m11_ + other.m11_, m12_ + other.m12_, m13_ + other.m13_, m14_ + other.m14_,
            m21_ + other.m21_, m22_ + other.m22_, m23_ + other.m23_, m24_ + other.m24_,
            m31_ + other.m31_, m32_ + other.m32_, m33_ + other.m33_, m34_ + other.m34_,
            m41_ + other.m41_, m42_ + other.m42_, m43_ + other.m43_, m44_ + other.m44_
        );
    }
    
    [[nodiscard]] Vec4<T> operator*(const Vec4<T>& v) const noexcept {
        return Vec4(
            m11_ * v.x() + m12_ * v.y() + m13_ * v.z() + m14_ * v.w(),
            m21_ * v.x() + m22_ * v.y() + m23_ * v.z() + m24_ * v.w(),
            m31_ * v.x() + m32_ * v.y() + m33_ * v.z() + m34_ * v.w(),
            m41_ * v.x() + m42_ * v.y() + m43_ * v.z() + m44_ * v.w()
        );
    }
    
    // Transform operations
    [[nodiscard]] Vec3<T> transform_point(const Vec3<T>& point) const noexcept {
        Vec4<T> result = (*this) * Vec4<T>(point, T{1});
        return result.xyz() / result.w();
    }
    
    [[nodiscard]] Vec3<T> transform_direction(const Vec3<T>& dir) const noexcept {
        Vec4<T> result = (*this) * Vec4<T>(dir, T{0});
        return result.xyz();
    }
    
private:
    T m11_, m12_, m13_, m14_;
    T m21_, m22_, m23_, m24_;
    T m31_, m32_, m33_, m34_;
    T m41_, m42_, m43_, m44_;
};

using Mat3f = Mat3<float>;
using Mat3d = Mat3<double>;
using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;

}  // namespace math
}  // namespace gw
