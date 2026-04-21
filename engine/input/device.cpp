// engine/input/device.cpp
#include "engine/input/device.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>

namespace gw::input {

DeviceRegistry::DeviceRegistry() = default;

bool DeviceRegistry::on_connected(const Device& d) {
    for (auto& existing : devices_) {
        if (existing.id == d.id) {
            existing = d;
            return false;
        }
    }
    devices_.push_back(d);
    return true;
}

bool DeviceRegistry::on_removed(DeviceId id) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
                           [id](const Device& d) { return d.id == id; });
    if (it == devices_.end()) return false;
    devices_.erase(it);
    return true;
}

const Device* DeviceRegistry::find(DeviceId id) const noexcept {
    for (const auto& d : devices_) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

uint32_t DeviceRegistry::detect_adaptive_flags(std::string_view name, uint32_t vid, uint32_t pid) noexcept {
    uint32_t caps = Cap_None;

    // Microsoft Xbox Adaptive Controller — VID 0x045E, specific PIDs.
    if (vid == 0x045E && (pid == 0x0B0A || pid == 0x0B10 || pid == 0x0B11)) {
        caps |= Cap_Adaptive;
    }

    // Name-based heuristics for devices we haven't whitelisted by VID/PID
    // (user-provided override possible via a future `assets/input/devices.toml`).
    std::string lower;
    lower.reserve(name.size());
    for (char c : name) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    if (lower.find("quadstick") != std::string::npos) {
        caps |= Cap_Quadstick | Cap_Adaptive;
    }
    if (lower.find("xbox adaptive") != std::string::npos) {
        caps |= Cap_Adaptive;
    }
    return caps;
}

// -----------------------------------------------------------------
// Processor pipeline
// -----------------------------------------------------------------

namespace {

float apply_curve(Curve c, float x) noexcept {
    switch (c) {
        case Curve::Linear:    return x;
        case Curve::Quadratic: return x * std::fabs(x);
        case Curve::Cubic:     return x * x * x;
    }
    return x;
}

float apply_per_axis_dz(float x, float inner, float outer) noexcept {
    const float ax = std::fabs(x);
    if (ax <= inner) return 0.0f;
    if (ax >= outer) return x < 0.0f ? -1.0f : 1.0f;
    const float span = std::max(outer - inner, 1e-6f);
    const float n = (ax - inner) / span;
    return (x < 0.0f) ? -n : n;
}

} // namespace

float process_scalar(const Processor& p, float raw, float& state) noexcept {
    float x = raw;
    x = apply_per_axis_dz(x, p.dz.inner, p.dz.outer);
    x = apply_curve(p.curve, x);
    if (p.invert) x = -x;
    x *= p.scale;
    x = std::clamp(x, p.clamp_min, p.clamp_max);
    if (p.smoothing > 0.0f && p.smoothing < 1.0f) {
        const float a = 1.0f - p.smoothing;
        state = state * p.smoothing + x * a;
        return state;
    }
    state = x;
    return x;
}

math::Vec2f process_stick(const Processor& p, math::Vec2f raw, math::Vec2f& state) noexcept {
    float x = raw.x();
    float y = raw.y();

    if (p.dz.radial) {
        const float r = std::sqrt(x * x + y * y);
        if (r <= p.dz.inner) {
            x = 0; y = 0;
        } else {
            const float span = std::max(p.dz.outer - p.dz.inner, 1e-6f);
            const float n = std::clamp((r - p.dz.inner) / span, 0.0f, 1.0f);
            const float scale = r > 1e-6f ? (n / r) : 0.0f;
            x *= scale;
            y *= scale;
        }
    } else {
        x = apply_per_axis_dz(x, p.dz.inner, p.dz.outer);
        y = apply_per_axis_dz(y, p.dz.inner, p.dz.outer);
    }
    x = apply_curve(p.curve, x);
    y = apply_curve(p.curve, y);
    if (p.invert) { x = -x; y = -y; }
    x *= p.scale; y *= p.scale;
    x = std::clamp(x, p.clamp_min, p.clamp_max);
    y = std::clamp(y, p.clamp_min, p.clamp_max);
    if (p.smoothing > 0.0f && p.smoothing < 1.0f) {
        const float a = 1.0f - p.smoothing;
        state = math::Vec2f(state.x() * p.smoothing + x * a,
                            state.y() * p.smoothing + y * a);
        return state;
    }
    state = math::Vec2f(x, y);
    return state;
}

} // namespace gw::input
