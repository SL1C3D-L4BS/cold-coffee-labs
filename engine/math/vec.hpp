#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <cmath>

namespace gw {
namespace math {

template<typename T>
class Vec2 {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    constexpr Vec2() noexcept : x_(T{}), y_(T{}) {}
    constexpr Vec2(T x, T y) noexcept : x_(x), y_(y) {}
    
    [[nodiscard]] T x() const noexcept { return x_; }
    [[nodiscard]] T y() const noexcept { return y_; }
    
    Vec2& operator+=(const Vec2& other) noexcept {
        x_ += other.x_;
        y_ += other.y_;
        return *this;
    }
    
    [[nodiscard]] Vec2 operator+(const Vec2& other) const noexcept {
        return Vec2(x_ + other.x_, y_ + other.y_);
    }
    
    [[nodiscard]] Vec2 operator-() const noexcept {
        return Vec2(-x_, -y_);
    }
    
    [[nodiscard]] Vec2 operator*(T scalar) const noexcept {
        return Vec2(x_ * scalar, y_ * scalar);
    }
    
    [[nodiscard]] T dot(const Vec2& other) const noexcept {
        return x_ * other.x_ + y_ * other.y_;
    }
    
    [[nodiscard]] T length_squared() const noexcept {
        return x_ * x_ + y_ * y_;
    }
    
    [[nodiscard]] T length() const noexcept {
        return std::sqrt(length_squared());
    }
    
private:
    T x_{};
    T y_{};
};

template<typename T>
class Vec3 {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    constexpr Vec3() noexcept : x_(T{}), y_(T{}), z_(T{}) {}
    constexpr Vec3(T x, T y, T z) noexcept : x_(x), y_(y), z_(z) {}
    
    [[nodiscard]] T x() const noexcept { return x_; }
    [[nodiscard]] T y() const noexcept { return y_; }
    [[nodiscard]] T z() const noexcept { return z_; }
    
    Vec3& operator+=(const Vec3& other) noexcept {
        x_ += other.x_;
        y_ += other.y_;
        z_ += other.z_;
        return *this;
    }
    
    [[nodiscard]] Vec3 operator+(const Vec3& other) const noexcept {
        return Vec3(x_ + other.x_, y_ + other.y_, z_ + other.z_);
    }
    
    [[nodiscard]] Vec3 operator-() const noexcept {
        return Vec3(-x_, -y_, -z_);
    }
    
    [[nodiscard]] Vec3 operator*(T scalar) const noexcept {
        return Vec3(x_ * scalar, y_ * scalar, z_ * scalar);
    }
    
    [[nodiscard]] T dot(const Vec3& other) const noexcept {
        return x_ * other.x_ + y_ * other.y_ + z_ * other.z_;
    }
    
    [[nodiscard]] T length_squared() const noexcept {
        return x_ * x_ + y_ * y_ + z_ * z_;
    }
    
    [[nodiscard]] T length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    [[nodiscard]] Vec2<T> xy() const noexcept {
        return Vec2<T>(x_, y_);
    }
    
private:
    T x_{};
    T y_{};
    T z_{};
};

// Type aliases for common use cases
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

}  // namespace math
}  // namespace gw
