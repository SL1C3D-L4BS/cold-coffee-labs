#pragma once
// engine/input/haptics.hpp — cross-device haptics router.
//
// Per GAG Motor/Basic "ensure that all haptics can be disabled or adjusted":
// `Haptics` exposes a global on/off + global volume + per-device toggle.
// The backend is never called when haptics are globally off (§2.6 of ADR-0020).

#include "engine/input/device.hpp"
#include "engine/input/input_backend.hpp"

#include <algorithm>
#include <unordered_map>

namespace gw::input {

class Haptics {
public:
    explicit Haptics(IInputBackend& backend) : backend_(backend) {}

    void set_global_enabled(bool enabled) noexcept { global_enabled_ = enabled; }
    [[nodiscard]] bool global_enabled() const noexcept { return global_enabled_; }

    void set_global_volume(float v) noexcept { global_volume_ = std::clamp(v, 0.0f, 1.0f); }
    [[nodiscard]] float global_volume() const noexcept { return global_volume_; }

    void set_device_enabled(DeviceId id, bool enabled) {
        per_device_[id.value] = enabled;
    }
    [[nodiscard]] bool device_enabled(DeviceId id) const {
        auto it = per_device_.find(id.value);
        return it == per_device_.end() ? true : it->second;
    }

    void play(DeviceId id, const HapticEvent& ev) {
        if (!global_enabled_) return;
        if (!device_enabled(id)) return;
        HapticEvent scaled = ev;
        scaled.low_freq      *= global_volume_;
        scaled.high_freq     *= global_volume_;
        scaled.left_trigger  *= global_volume_;
        scaled.right_trigger *= global_volume_;
        backend_.rumble(id, scaled);
    }

private:
    IInputBackend&                           backend_;
    bool                                     global_enabled_{true};
    float                                    global_volume_{1.0f};
    std::unordered_map<uint64_t, bool>       per_device_{};
};

} // namespace gw::input
