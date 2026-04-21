// tests/unit/editor/selection_test.cpp
// Phase 7 gate: SelectionContext correctness — spec §17.
//
// <ostream> before doctest: MSVC STL 14.44 + clang-cl compat.
// See command_stack_test.cpp for the full explanation.
#include <ostream>
#include <doctest/doctest.h>
#include "editor/selection/selection_context.hpp"

using namespace gw::editor;

TEST_CASE("SelectionContext — single select clears previous") {
    SelectionContext sel;
    sel.set(1u);
    sel.set(2u);
    CHECK(sel.count() == 1);
    CHECK(sel.primary() == 2u);
    CHECK_FALSE(sel.is_selected(1u));
}

TEST_CASE("SelectionContext — toggle adds absent entity") {
    SelectionContext sel;
    sel.toggle(10u);
    CHECK(sel.count() == 1);
    CHECK(sel.is_selected(10u));
}

TEST_CASE("SelectionContext — toggle removes present entity") {
    SelectionContext sel;
    sel.toggle(10u);
    sel.toggle(10u);
    CHECK(sel.empty());
}

TEST_CASE("SelectionContext — multi-select via toggle") {
    SelectionContext sel;
    sel.toggle(1u); sel.toggle(2u); sel.toggle(3u);
    CHECK(sel.count() == 3);
    CHECK(sel.is_selected(1u));
    CHECK(sel.is_selected(2u));
    CHECK(sel.is_selected(3u));
}

TEST_CASE("SelectionContext — clear empties the selection") {
    SelectionContext sel;
    sel.set(5u);
    sel.clear();
    CHECK(sel.empty());
    CHECK(sel.primary() == kNullEntity);
}

TEST_CASE("SelectionContext — set(nullEntity) clears") {
    SelectionContext sel;
    sel.set(7u);
    sel.set(kNullEntity);
    CHECK(sel.empty());
}

TEST_CASE("SelectionContext — on_changed signal fires on set") {
    SelectionContext sel;
    int call_count = 0;
    sel.subscribe([&](const SelectionContext&){ ++call_count; });
    sel.set(1u);
    sel.set(2u);
    sel.clear();
    CHECK(call_count == 3);
}

TEST_CASE("SelectionContext — selected() span correct") {
    SelectionContext sel;
    sel.toggle(100u); sel.toggle(200u);
    auto span = sel.selected();
    CHECK(span.size() == 2);
}
