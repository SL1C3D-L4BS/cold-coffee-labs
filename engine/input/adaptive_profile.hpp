#pragma once
// engine/input/adaptive_profile.hpp — Xbox Adaptive / Quadstick auto-profile.
//
// When a device with Cap_Adaptive or Cap_Quadstick appears, we overlay a
// predetermined ActionMap profile on top of the active map (ADR-0022 §2,
// accessibility self-check §M.2). The profile is still user-overridable
// from the rebind UI; this just provides sane defaults so the controller
// is playable out of the box.

#include "engine/input/action_map_toml.hpp"
#include "engine/input/device.hpp"

#include <string>

namespace gw::input {

struct AdaptiveProfile {
    std::string name;                       // "xbox_adaptive", "quadstick"
    ActionMap   overlay;                    // layered on top of user map
    bool        auto_enable_hold_to_toggle{true};
    bool        auto_enable_scanner{false};
};

// Built-in profile for the Microsoft Xbox Adaptive Controller.
[[nodiscard]] AdaptiveProfile xbox_adaptive_profile();

// Built-in profile for Quadstick devices.
[[nodiscard]] AdaptiveProfile quadstick_profile();

// Picks the best profile for a device, or nullptr if none applies.
[[nodiscard]] const AdaptiveProfile* pick_profile(const Device& d) noexcept;

// Overlay `profile.overlay` onto `target`. Actions in the overlay replace
// matching actions in the target by name. Accessibility flags from the
// profile (e.g. hold-to-toggle) are OR'd in.
void apply_profile(ActionMap& target, const AdaptiveProfile& profile) noexcept;

} // namespace gw::input
