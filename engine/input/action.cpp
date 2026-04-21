// engine/input/action.cpp
#include "engine/input/action.hpp"

#include <algorithm>
#include <cmath>

namespace gw::input {

Action& ActionSet::add(Action a) {
    for (auto& existing : actions_) {
        if (existing.name == a.name) {
            existing = std::move(a);
            return existing;
        }
    }
    actions_.push_back(std::move(a));
    return actions_.back();
}

Action* ActionSet::find(std::string_view name) noexcept {
    for (auto& a : actions_) if (a.name == name) return &a;
    return nullptr;
}
const Action* ActionSet::find(std::string_view name) const noexcept {
    for (const auto& a : actions_) if (a.name == name) return &a;
    return nullptr;
}

namespace {

bool key_down_id(const RawSnapshot& s, uint32_t code) noexcept {
    if (code >= 256) return false;
    return (s.keys[code / 64] & (1ULL << (code % 64))) != 0;
}

float gamepad_button_strength(const RawSnapshot& s, uint32_t code) noexcept {
    for (const auto& g : s.gamepads) {
        if (!g.present) continue;
        if ((g.buttons & (1u << code)) != 0) return 1.0f;
    }
    return 0.0f;
}

float gamepad_axis_value(const RawSnapshot& s, uint32_t code) noexcept {
    if (code >= static_cast<uint32_t>(GamepadAxis::Count)) return 0.0f;
    for (const auto& g : s.gamepads) {
        if (!g.present) continue;
        return g.axes[code];
    }
    return 0.0f;
}

bool modifiers_match(ModifierMask want, ModifierMask actual) noexcept {
    if (want == 0) return true;
    return (actual & want) == want;
}

} // namespace

float evaluate_binding(const Binding& b, const RawSnapshot& snap) noexcept {
    switch (b.source) {
        case BindingSource::Keyboard:
            if (!modifiers_match(b.modifiers, snap.modifiers)) return 0.0f;
            return key_down_id(snap, b.code) ? 1.0f : 0.0f;

        case BindingSource::Mouse:
            return (snap.mouse_buttons & (1u << b.code)) ? 1.0f : 0.0f;

        case BindingSource::MouseAxis:
            if (b.code == 0) return snap.mouse_dx;
            if (b.code == 1) return snap.mouse_dy;
            if (b.code == 2) return snap.mouse_wheel;
            return 0.0f;

        case BindingSource::GamepadButton: {
            const float v = gamepad_button_strength(snap, b.code);
            return (v >= b.threshold) ? v : 0.0f;
        }

        case BindingSource::GamepadAxis: {
            const float v = gamepad_axis_value(snap, b.code);
            return b.inverted ? -v : v;
        }

        case BindingSource::GamepadGyro:
            if (snap.gamepads.empty()) return 0.0f;
            if (b.code == 0) return snap.gamepads[0].gyro.x();
            if (b.code == 1) return snap.gamepads[0].gyro.y();
            if (b.code == 2) return snap.gamepads[0].gyro.z();
            return 0.0f;

        default:
            return 0.0f;
    }
}

math::Vec2f evaluate_composite(const Binding& b, const RawSnapshot& snap) noexcept {
    if (b.source != BindingSource::CompositeWASD) return {};
    const float up    = key_down_id(snap, b.composite_keys[0]) ? 1.0f : 0.0f;
    const float down  = key_down_id(snap, b.composite_keys[1]) ? 1.0f : 0.0f;
    const float left  = key_down_id(snap, b.composite_keys[2]) ? 1.0f : 0.0f;
    const float right = key_down_id(snap, b.composite_keys[3]) ? 1.0f : 0.0f;
    return math::Vec2f(right - left, up - down);
}

namespace {

// Hold-to-toggle transducer (ADR-0021 §2.7).
constexpr float kHoldTapThresholdMs = 250.0f;
constexpr float kDoubleTapWindowMs  = 350.0f;

void apply_hold_to_toggle(Action& a, bool raw_down, float now_ms) noexcept {
    if (!a.accessibility.hold_to_toggle) {
        a.state.latched = false;
        return;
    }
    const bool was = a.state.last_pressed;
    const bool rising = !was && raw_down;
    const bool falling = was && !raw_down;
    if (rising) {
        // Detect double-tap while latched: press within window un-latches and
        // tells the following falling edge to skip its re-latch.
        if (a.state.latched && (now_ms - a.state.last_release_time_ms) <= kDoubleTapWindowMs) {
            a.state.latched = false;
            a.state.suppress_next_latch = true;
        }
        a.state.last_press_time_ms = now_ms;
    }
    if (falling) {
        const float held = now_ms - a.state.last_press_time_ms;
        if (a.state.suppress_next_latch) {
            a.state.suppress_next_latch = false;
        } else if (held < kHoldTapThresholdMs && !a.state.latched) {
            a.state.latched = true;
        } else if (held >= kHoldTapThresholdMs) {
            a.state.latched = false;
        }
        a.state.last_release_time_ms = now_ms;
    }
    a.state.last_pressed = raw_down;
}

void apply_auto_repeat(Action& a, bool raw_down, float now_ms, bool& out_pressed) noexcept {
    if (a.accessibility.auto_repeat_ms <= 0.0f) {
        out_pressed = raw_down || a.state.latched;
        return;
    }
    if (raw_down || a.state.latched) {
        if (a.state.last_repeat_time_ms < 0.0f ||
            (now_ms - a.state.last_repeat_time_ms) >= a.accessibility.auto_repeat_ms) {
            a.state.last_repeat_time_ms = now_ms;
            out_pressed = true;
        } else {
            out_pressed = false;
        }
    } else {
        a.state.last_repeat_time_ms = -1000.0f;
        out_pressed = false;
    }
}

bool passes_cooldown(const Action& a, float now_ms) noexcept {
    if (a.accessibility.repeat_cooldown_ms <= 0.0f) return true;
    return (now_ms - a.state.last_activation_time_ms) >= a.accessibility.repeat_cooldown_ms;
}

} // namespace

void evaluate_action(Action& a, const RawSnapshot& snap, float now_ms) noexcept {
    // Fold all bindings into either a boolean or a max-strength/additive
    // value depending on output kind.
    if (a.output == ActionOutputKind::Bool) {
        float strength = 0.0f;
        for (const auto& b : a.bindings) {
            strength = std::max(strength, evaluate_binding(b, snap));
        }
        const bool raw_down = strength > 0.0f;
        apply_hold_to_toggle(a, raw_down, now_ms);
        bool out = raw_down;
        apply_auto_repeat(a, raw_down, now_ms, out);
        if (out && passes_cooldown(a, now_ms)) {
            a.state.last_activation_time_ms = now_ms;
        } else if (!passes_cooldown(a, now_ms)) {
            out = false;
        }
        a.value.kind = ActionOutputKind::Bool;
        a.value.b = out;
        return;
    }
    if (a.output == ActionOutputKind::Float) {
        float v = 0.0f;
        for (const auto& b : a.bindings) {
            const float s = evaluate_binding(b, snap);
            if (std::fabs(s) > std::fabs(v)) v = s;
        }
        float out = process_scalar(a.processor, v, a.state.scalar_state);
        a.value.kind = ActionOutputKind::Float;
        a.value.f = out;
        return;
    }
    if (a.output == ActionOutputKind::Vec2) {
        math::Vec2f sum{0.0f, 0.0f};
        for (const auto& b : a.bindings) {
            if (b.source == BindingSource::CompositeWASD) {
                auto v = evaluate_composite(b, snap);
                sum = math::Vec2f(sum.x() + v.x(), sum.y() + v.y());
            } else if (b.source == BindingSource::GamepadAxis) {
                // Stick axes come in pairs (code = 0/1 = left, 2/3 = right).
                if (b.code == 0 || b.code == 2) {
                    sum = math::Vec2f(sum.x() + gamepad_axis_value(snap, b.code), sum.y());
                } else if (b.code == 1 || b.code == 3) {
                    sum = math::Vec2f(sum.x(), sum.y() + gamepad_axis_value(snap, b.code));
                }
            }
        }
        // Clamp each axis to [-1, 1] before processor.
        sum = math::Vec2f(std::clamp(sum.x(), -1.0f, 1.0f), std::clamp(sum.y(), -1.0f, 1.0f));
        auto out = process_stick(a.processor, sum, a.state.vec2_state);
        a.value.kind = ActionOutputKind::Vec2;
        a.value.v2 = out;
        return;
    }
    // Vec3 / U32 — minimal support: take first binding's triple or code.
    if (a.output == ActionOutputKind::Vec3) {
        if (!a.bindings.empty() && a.bindings[0].source == BindingSource::GamepadGyro) {
            for (const auto& g : snap.gamepads) {
                if (g.present) {
                    a.value.kind = ActionOutputKind::Vec3;
                    a.value.v3 = g.gyro;
                    return;
                }
            }
        }
        a.value.kind = ActionOutputKind::Vec3;
        a.value.v3 = math::Vec3f(0.0f, 0.0f, 0.0f);
        return;
    }
    if (a.output == ActionOutputKind::U32) {
        // Pick highest-strength binding index.
        float best = 0.0f; uint32_t idx = 0;
        for (uint32_t i = 0; i < a.bindings.size(); ++i) {
            const float s = evaluate_binding(a.bindings[i], snap);
            if (s > best) { best = s; idx = i + 1; }
        }
        a.value.kind = ActionOutputKind::U32;
        a.value.u = idx;
        return;
    }
}

void evaluate_set(ActionSet& set, const RawSnapshot& snap, float now_ms) noexcept {
    for (auto& a : set.actions()) evaluate_action(a, snap, now_ms);
}

} // namespace gw::input
