// tests/unit/input/rebind_test.cpp — ADR-0021 rebind conflict detection.

#include <doctest/doctest.h>

#include "engine/input/action_map_toml.hpp"
#include "engine/input/rebind.hpp"

using namespace gw::input;

namespace {

ActionMap map_with_two_bound_actions() {
    ActionMap m;
    m.sets.emplace_back("gameplay");
    Action fire{};
    fire.name = "fire";
    fire.output = ActionOutputKind::Bool;
    Binding b1{};
    b1.source = BindingSource::Keyboard;
    b1.code = static_cast<uint32_t>(KeyCode::Space);
    fire.bindings.push_back(b1);
    m.sets[0].add(fire);

    Action jump{};
    jump.name = "jump";
    jump.output = ActionOutputKind::Bool;
    Binding b2{};
    b2.source = BindingSource::Keyboard;
    b2.code = static_cast<uint32_t>(KeyCode::W);
    jump.bindings.push_back(b2);
    m.sets[0].add(jump);
    return m;
}

} // namespace

TEST_CASE("detect_conflicts: finds duplicate binding on another action") {
    auto m = map_with_two_bound_actions();
    Binding new_b{};
    new_b.source = BindingSource::Keyboard;
    new_b.code = static_cast<uint32_t>(KeyCode::W);
    auto conflicts = detect_conflicts(m, "fire", new_b);
    REQUIRE(conflicts.size() == 1);
    CHECK(conflicts[0].action_name == "jump");
}

TEST_CASE("detect_conflicts: no conflicts when editing same action") {
    auto m = map_with_two_bound_actions();
    Binding new_b{};
    new_b.source = BindingSource::Keyboard;
    new_b.code = static_cast<uint32_t>(KeyCode::Space);
    auto conflicts = detect_conflicts(m, "fire", new_b);
    CHECK(conflicts.empty());
}

TEST_CASE("binding_from_event: accepts key / mouse / gamepad") {
    RawEvent e{};
    e.kind = RawEventKind::KeyDown;
    e.code = static_cast<uint32_t>(KeyCode::F);
    auto b = binding_from_event(e);
    CHECK(b.source == BindingSource::Keyboard);
    CHECK(b.code == static_cast<uint32_t>(KeyCode::F));

    RawEvent gb{};
    gb.kind = RawEventKind::GamepadButtonDown;
    gb.code = static_cast<uint32_t>(GamepadButton::A);
    auto gbind = binding_from_event(gb);
    CHECK(gbind.source == BindingSource::GamepadButton);
}

TEST_CASE("binding_from_event: axis requires large deflection") {
    RawEvent e{};
    e.kind = RawEventKind::GamepadAxis_;
    e.code = 0;
    e.fval[0] = 0.1f;  // below threshold
    CHECK(binding_from_event(e).source == BindingSource::None);

    e.fval[0] = 0.9f;
    CHECK(binding_from_event(e).source == BindingSource::GamepadAxis);
}

TEST_CASE("apply_rebind: replaces first binding") {
    ActionSet set{"gameplay"};
    Action fire{};
    fire.name = "fire";
    fire.output = ActionOutputKind::Bool;
    Binding b0{};
    b0.source = BindingSource::Keyboard;
    b0.code = static_cast<uint32_t>(KeyCode::Space);
    fire.bindings.push_back(b0);
    set.add(fire);

    Binding b1{};
    b1.source = BindingSource::Keyboard;
    b1.code = static_cast<uint32_t>(KeyCode::Return);
    CHECK(apply_rebind(set, "fire", b1));
    CHECK(set.find("fire")->bindings.front().code == static_cast<uint32_t>(KeyCode::Return));
}
