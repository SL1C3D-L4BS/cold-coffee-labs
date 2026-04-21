// tests/unit/input/scanner_test.cpp — ADR-0021 single-switch scanner.

#include <doctest/doctest.h>

#include "engine/input/scanner.hpp"

using namespace gw::input;

namespace {

Action make_scan_action(const char* name, int scan_index) {
    Action a{};
    a.name = name;
    a.output = ActionOutputKind::Bool;
    a.accessibility.scan_index = scan_index;
    return a;
}

} // namespace

TEST_CASE("Scanner: candidates sorted by scan_index") {
    ActionSet set{"gameplay"};
    set.add(make_scan_action("c", 2));
    set.add(make_scan_action("a", 0));
    set.add(make_scan_action("b", 1));

    ContextStack stack;
    stack.push(&set);
    SingleSwitchScanner s;
    s.set_candidates(gather_scan_candidates(stack));
    REQUIRE(s.candidate_count() == 3);
    CHECK(s.highlighted()->name == "a");
}

TEST_CASE("Scanner: tick advances after scan_rate_ms") {
    ActionSet set{"gameplay"};
    set.add(make_scan_action("a", 0));
    set.add(make_scan_action("b", 1));
    ContextStack stack;
    stack.push(&set);
    SingleSwitchScanner s;
    s.configure({.scan_rate_ms = 100.0f});
    s.set_candidates(gather_scan_candidates(stack));

    CHECK(s.highlighted()->name == "a");
    s.tick(0.0f);     // initialise timer
    s.tick(50.0f);    // still early
    CHECK(s.highlighted()->name == "a");
    s.tick(150.0f);
    CHECK(s.highlighted()->name == "b");
    s.tick(300.0f);
    CHECK(s.highlighted()->name == "a");  // wraps
}

TEST_CASE("Scanner: activate fires the highlighted action") {
    ActionSet set{"gameplay"};
    set.add(make_scan_action("fire", 0));
    ContextStack stack;
    stack.push(&set);
    SingleSwitchScanner s;
    s.set_candidates(gather_scan_candidates(stack));
    auto* a = s.activate(100.0f);
    REQUIRE(a != nullptr);
    CHECK(a->value.b == true);
    CHECK(a->state.last_activation_time_ms == doctest::Approx(100.0f));
}

TEST_CASE("Scanner: no candidates → no-op") {
    SingleSwitchScanner s;
    CHECK(s.candidate_count() == 0);
    CHECK_FALSE(s.tick(1000.0f));
    CHECK(s.activate(0.0f) == nullptr);
}
