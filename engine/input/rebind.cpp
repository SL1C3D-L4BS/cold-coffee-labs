// engine/input/rebind.cpp
#include "engine/input/rebind.hpp"

namespace gw::input {

namespace {

bool same_slot(const Binding& a, const Binding& b) noexcept {
    if (a.source != b.source) return false;
    if (a.code != b.code) return false;
    if (a.modifiers != b.modifiers) return false;
    return true;
}

} // namespace

std::vector<RebindConflict> detect_conflicts(const ActionMap& map,
                                             std::string_view editing_action,
                                             const Binding&   proposed) noexcept {
    std::vector<RebindConflict> out;
    if (proposed.source == BindingSource::None) return out;
    for (const auto& set : map.sets) {
        for (const auto& a : set.actions()) {
            if (a.name == editing_action) continue;
            for (const auto& b : a.bindings) {
                if (same_slot(b, proposed)) {
                    out.push_back({set.name(), a.name, b});
                }
            }
        }
    }
    return out;
}

Binding binding_from_event(const RawEvent& e) noexcept {
    Binding b{};
    switch (e.kind) {
        case RawEventKind::KeyDown:
            b.source = BindingSource::Keyboard;
            b.code = e.code;
            b.modifiers = e.modifiers;
            break;
        case RawEventKind::MouseButtonDown:
            b.source = BindingSource::Mouse;
            b.code = e.code;
            break;
        case RawEventKind::GamepadButtonDown:
            b.source = BindingSource::GamepadButton;
            b.code = e.code;
            break;
        case RawEventKind::GamepadAxis_:
            // Only accept axes that swing past a generous threshold, so the
            // resting position doesn't accidentally bind.
            if (e.fval[0] < -0.7f || e.fval[0] > 0.7f) {
                b.source = BindingSource::GamepadAxis;
                b.code = e.code;
                b.threshold = 0.5f;
            }
            break;
        case RawEventKind::GamepadGyro:
            b.source = BindingSource::GamepadGyro;
            b.code = e.code;
            break;
        default:
            break;
    }
    return b;
}

bool apply_rebind(ActionSet& set, std::string_view action_name, const Binding& b) noexcept {
    auto* a = set.find(action_name);
    if (a == nullptr) return false;
    if (a->bindings.empty()) {
        a->bindings.push_back(b);
    } else {
        a->bindings.front() = b;
    }
    return true;
}

} // namespace gw::input
