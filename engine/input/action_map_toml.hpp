#pragma once
// engine/input/action_map_toml.hpp — load / save action maps from TOML.
//
// Schema per ADR-0021 §2.4. The parser is deliberately minimal (our own
// file, no third-party dep) — handles sections, key=value pairs (scalars,
// strings, arrays of tables for bindings).

#include "engine/input/action.hpp"

#include <string>
#include <vector>

namespace gw::input {

struct ActionMap {
    std::vector<ActionSet> sets;
    // Accessibility defaults (applied to every Action unless overridden).
    ActionAccessibility defaults{};

    [[nodiscard]] ActionSet* find_set(std::string_view name) noexcept;
    [[nodiscard]] const ActionSet* find_set(std::string_view name) const noexcept;
};

[[nodiscard]] bool load_action_map_toml(const std::string& source, ActionMap& out);
[[nodiscard]] std::string save_action_map_toml(const ActionMap& map);

// Canonicalisation helpers — stable across saves (ADR-0021 §2.4 round-trip).
[[nodiscard]] std::string binding_to_string(const Binding&);
[[nodiscard]] Binding binding_from_string(const std::string&);

[[nodiscard]] std::string keycode_to_string(KeyCode);
[[nodiscard]] KeyCode     keycode_from_string(std::string_view);
[[nodiscard]] std::string gamepad_button_to_string(GamepadButton);
[[nodiscard]] GamepadButton gamepad_button_from_string(std::string_view);
[[nodiscard]] std::string gamepad_axis_to_string(GamepadAxis);
[[nodiscard]] GamepadAxis gamepad_axis_from_string(std::string_view);

} // namespace gw::input
