#pragma once
// engine/input/device.hpp — device registry + processor pipeline.

#include "engine/input/input_types.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace gw::input {

struct Device {
    DeviceId    id{};
    DeviceKind  kind{DeviceKind::Unknown};
    uint32_t    caps{0};
    std::array<char, 64> name{};  // null-terminated display name

    [[nodiscard]] bool has(DeviceCap c) const noexcept { return has_cap(caps, c); }
};

class DeviceRegistry {
public:
    DeviceRegistry();
    ~DeviceRegistry() = default;

    // Returns true if the device is newly added.
    bool on_connected(const Device& d);
    // Returns true if the device was removed.
    bool on_removed(DeviceId id);

    [[nodiscard]] const Device* find(DeviceId id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return devices_.size(); }
    [[nodiscard]] std::vector<Device>& devices() noexcept { return devices_; }
    [[nodiscard]] const std::vector<Device>& devices() const noexcept { return devices_; }

    // Adaptive detection — Microsoft VID 0x045E with Adaptive product IDs,
    // or devices that carry "quadstick" in their name.
    static uint32_t detect_adaptive_flags(std::string_view name, uint32_t vid, uint32_t pid) noexcept;

private:
    std::vector<Device> devices_;
};

// -----------------------------------------------------------------
// Processor pipeline (Deadzone → Curve → Invert → Scale → Clamp → Smooth)
// -----------------------------------------------------------------

struct Deadzone {
    float inner{0.15f};
    float outer{1.0f};
    bool  radial{true};   // for 2D sticks; false = per-axis
};

enum class Curve : uint8_t { Linear = 0, Quadratic, Cubic };

struct Processor {
    Deadzone dz{};
    Curve    curve{Curve::Linear};
    bool     invert{false};
    float    scale{1.0f};
    float    clamp_min{-1.0f};
    float    clamp_max{1.0f};
    float    smoothing{0.0f};  // exponential smoothing [0, 1)
};

// Scalar (per-axis)
[[nodiscard]] float process_scalar(const Processor& p, float raw, float& state) noexcept;

// 2D stick
[[nodiscard]] math::Vec2f process_stick(const Processor& p, math::Vec2f raw,
                                        math::Vec2f& state) noexcept;

} // namespace gw::input
