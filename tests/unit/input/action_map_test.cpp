// tests/unit/input/action_map_test.cpp — ADR-0021 action map evaluation.

#include <doctest/doctest.h>

#include "engine/input/action.hpp"
#include "engine/input/action_map_toml.hpp"

#include <string>

using namespace gw::input;

namespace {

RawSnapshot snapshot_with_key_down(KeyCode k) {
    RawSnapshot s{};
    const auto code = static_cast<uint32_t>(k);
    s.keys[code / 64] |= (1ULL << (code % 64));
    return s;
}

} // namespace

TEST_CASE("Binding (keyboard): evaluates to 1 when key is down") {
    Binding b{};
    b.source = BindingSource::Keyboard;
    b.code = static_cast<uint32_t>(KeyCode::Space);
    auto s = snapshot_with_key_down(KeyCode::Space);
    CHECK(evaluate_binding(b, s) == 1.0f);
}

TEST_CASE("CompositeWASD: right-up diagonal") {
    Binding b{};
    b.source = BindingSource::CompositeWASD;
    b.composite_keys[0] = static_cast<uint16_t>(KeyCode::W);
    b.composite_keys[1] = static_cast<uint16_t>(KeyCode::S);
    b.composite_keys[2] = static_cast<uint16_t>(KeyCode::A);
    b.composite_keys[3] = static_cast<uint16_t>(KeyCode::D);

    RawSnapshot s{};
    const auto w = static_cast<uint32_t>(KeyCode::W);
    const auto d = static_cast<uint32_t>(KeyCode::D);
    s.keys[w / 64] |= (1ULL << (w % 64));
    s.keys[d / 64] |= (1ULL << (d % 64));
    auto v = evaluate_composite(b, s);
    CHECK(v.x() == doctest::Approx(1.0f));
    CHECK(v.y() == doctest::Approx(1.0f));
}

TEST_CASE("Action (Bool): fires when binding is pressed") {
    Action a{};
    a.name = "jump";
    a.output = ActionOutputKind::Bool;
    Binding b{};
    b.source = BindingSource::Keyboard;
    b.code = static_cast<uint32_t>(KeyCode::Space);
    a.bindings.push_back(b);

    auto s = snapshot_with_key_down(KeyCode::Space);
    evaluate_action(a, s, 0.0f);
    CHECK(a.value.b == true);
}

TEST_CASE("Action (hold-to-toggle): tap latches, re-tap releases") {
    Action a{};
    a.output = ActionOutputKind::Bool;
    a.accessibility.hold_to_toggle = true;
    Binding b{};
    b.source = BindingSource::Keyboard;
    b.code = static_cast<uint32_t>(KeyCode::Space);
    a.bindings.push_back(b);

    // Tap (press + release within 100 ms → latch).
    auto down = snapshot_with_key_down(KeyCode::Space);
    evaluate_action(a, down, 0.0f);
    CHECK(a.value.b == true);
    RawSnapshot up{};
    evaluate_action(a, up, 100.0f);
    CHECK(a.state.latched);
    CHECK(a.value.b == true);    // still firing while latched

    // Double-tap to release.
    evaluate_action(a, down, 200.0f);
    evaluate_action(a, up,   210.0f);
    CHECK_FALSE(a.state.latched);
}

TEST_CASE("ActionMap TOML: save / load round-trip") {
    ActionMap m;
    m.sets.emplace_back("gameplay");
    Action jump{};
    jump.name = "jump";
    jump.output = ActionOutputKind::Bool;
    Binding b{};
    b.source = BindingSource::Keyboard;
    b.code = static_cast<uint32_t>(KeyCode::Space);
    jump.bindings.push_back(b);
    m.sets[0].add(jump);

    auto toml = save_action_map_toml(m);
    CHECK(toml.find("[action.jump]") != std::string::npos);

    ActionMap loaded;
    REQUIRE(load_action_map_toml(toml, loaded));
    auto* set = loaded.find_set("gameplay");
    REQUIRE(set != nullptr);
    auto* action = set->find("jump");
    REQUIRE(action != nullptr);
    REQUIRE(action->bindings.size() == 1);
    CHECK(action->bindings[0].source == BindingSource::Keyboard);
    CHECK(action->bindings[0].code == static_cast<uint32_t>(KeyCode::Space));
}

TEST_CASE("ContextStack: top-of-stack wins in resolution") {
    ActionSet gameplay{"gameplay"};
    ActionSet menu{"menu"};

    Action a_game{};
    a_game.name = "fire";
    a_game.output = ActionOutputKind::Bool;
    gameplay.add(a_game);

    Action a_menu{};
    a_menu.name = "fire";
    a_menu.output = ActionOutputKind::Bool;
    menu.add(a_menu);

    ContextStack stack;
    stack.push(&gameplay);
    stack.push(&menu);
    // Caller walks top-down; we just sanity-check order.
    REQUIRE(stack.stack().size() == 2);
    CHECK(stack.stack().back() == &menu);
}
