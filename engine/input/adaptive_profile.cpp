// engine/input/adaptive_profile.cpp
#include "engine/input/adaptive_profile.hpp"

namespace gw::input {

namespace {

AdaptiveProfile make_xbox_adaptive() {
    AdaptiveProfile p;
    p.name = "xbox_adaptive";
    p.auto_enable_hold_to_toggle = true;
    p.auto_enable_scanner = false;

    ActionSet gameplay{"gameplay"};

    auto add_bool_action = [&](const char* name, GamepadButton btn) {
        Action a{};
        a.name = name;
        a.output = ActionOutputKind::Bool;
        a.accessibility.hold_to_toggle = true;
        Binding b{};
        b.source = BindingSource::GamepadButton;
        b.code = static_cast<uint32_t>(btn);
        a.bindings.push_back(b);
        gameplay.add(a);
    };

    add_bool_action("fire",     GamepadButton::A);
    add_bool_action("jump",     GamepadButton::B);
    add_bool_action("interact", GamepadButton::X);
    add_bool_action("reload",   GamepadButton::Y);

    p.overlay.sets.push_back(std::move(gameplay));
    p.overlay.defaults.hold_to_toggle = true;
    p.overlay.defaults.repeat_cooldown_ms = 250.0f;
    return p;
}

AdaptiveProfile make_quadstick() {
    AdaptiveProfile p;
    p.name = "quadstick";
    p.auto_enable_hold_to_toggle = true;
    p.auto_enable_scanner = true;

    ActionSet gameplay{"gameplay"};

    // Quadstick exposes four pressure pads + stick. Map stick to movement,
    // pads to fire / jump / interact / reload with scan_index set so the
    // single-switch scanner can cycle through them if needed.
    {
        Action move{};
        move.name = "move";
        move.output = ActionOutputKind::Vec2;
        Binding stick{};
        stick.source = BindingSource::GamepadAxis;
        stick.code = static_cast<uint32_t>(GamepadAxis::LeftX);
        move.bindings.push_back(stick);
        stick.code = static_cast<uint32_t>(GamepadAxis::LeftY);
        move.bindings.push_back(stick);
        move.accessibility.repeat_cooldown_ms = 0.0f;
        gameplay.add(move);
    }

    int scan = 0;
    auto add_scan_action = [&](const char* name, GamepadButton btn) {
        Action a{};
        a.name = name;
        a.output = ActionOutputKind::Bool;
        a.accessibility.hold_to_toggle = true;
        a.accessibility.scan_index = scan++;
        a.accessibility.repeat_cooldown_ms = 250.0f;
        Binding b{};
        b.source = BindingSource::GamepadButton;
        b.code = static_cast<uint32_t>(btn);
        a.bindings.push_back(b);
        gameplay.add(a);
    };
    add_scan_action("fire",     GamepadButton::A);
    add_scan_action("jump",     GamepadButton::B);
    add_scan_action("interact", GamepadButton::X);
    add_scan_action("reload",   GamepadButton::Y);

    p.overlay.sets.push_back(std::move(gameplay));
    p.overlay.defaults.hold_to_toggle = true;
    p.overlay.defaults.repeat_cooldown_ms = 250.0f;
    return p;
}

} // namespace

AdaptiveProfile xbox_adaptive_profile() { return make_xbox_adaptive(); }
AdaptiveProfile quadstick_profile()     { return make_quadstick(); }

const AdaptiveProfile* pick_profile(const Device& d) noexcept {
    static const AdaptiveProfile kQuad    = make_quadstick();
    static const AdaptiveProfile kAdaptive = make_xbox_adaptive();
    if (d.has(Cap_Quadstick)) return &kQuad;
    if (d.has(Cap_Adaptive))  return &kAdaptive;
    return nullptr;
}

void apply_profile(ActionMap& target, const AdaptiveProfile& profile) noexcept {
    // Merge defaults (OR-in).
    if (profile.overlay.defaults.hold_to_toggle) target.defaults.hold_to_toggle = true;
    if (profile.overlay.defaults.repeat_cooldown_ms > target.defaults.repeat_cooldown_ms) {
        target.defaults.repeat_cooldown_ms = profile.overlay.defaults.repeat_cooldown_ms;
    }
    if (profile.overlay.defaults.auto_repeat_ms > target.defaults.auto_repeat_ms) {
        target.defaults.auto_repeat_ms = profile.overlay.defaults.auto_repeat_ms;
    }
    // Merge sets and actions (name-based replacement).
    for (const auto& overlay_set : profile.overlay.sets) {
        ActionSet* tgt = target.find_set(overlay_set.name());
        if (!tgt) {
            target.sets.emplace_back(overlay_set.name());
            tgt = &target.sets.back();
        }
        for (const auto& a : overlay_set.actions()) tgt->add(a);
    }
}

} // namespace gw::input
