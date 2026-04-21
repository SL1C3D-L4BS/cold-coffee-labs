#pragma once
// engine/input/rebind.hpp — runtime rebinding helpers (ADR-0021 §2.5).
//
// Rebind UX flow:
//   1. Caller invokes `begin_rebind(action_name)` — system clears that
//      action's bindings (or the one being edited).
//   2. Next Raw event that matches a "bindable" source produces a new Binding.
//   3. `finalize_rebind` installs the new binding; conflict detection runs
//      against the whole ActionMap and returns a list of conflicts so the UI
//      can prompt.

#include "engine/input/action.hpp"
#include "engine/input/action_map_toml.hpp"

#include <string>
#include <vector>

namespace gw::input {

struct RebindConflict {
    std::string set_name;
    std::string action_name;
    Binding     offending;
};

// Returns conflicts where the same (source, code, modifiers) is already bound
// elsewhere in the map (excluding the action being rebound).
[[nodiscard]] std::vector<RebindConflict> detect_conflicts(
    const ActionMap& map,
    std::string_view editing_action,
    const Binding&   proposed) noexcept;

// Turn a single RawEvent into a Binding, if the event is a bindable source
// (key down, mouse button down, gamepad button/axis/gyro). Returns an
// empty Binding with source=None on failure.
[[nodiscard]] Binding binding_from_event(const RawEvent& e) noexcept;

// Replace the first binding of `action_name` in `set`, or append if none.
bool apply_rebind(ActionSet& set, std::string_view action_name, const Binding& b) noexcept;

} // namespace gw::input
