// tests/unit/editor/selection_test.cpp
// Phase 7 gate: SelectionContext correctness — spec §17.
//
// <ostream> before doctest: MSVC STL 14.44 + clang-cl compat.
// See command_stack_test.cpp for the full explanation.
#include <ostream>
#include <doctest/doctest.h>
#include "editor/selection/selection_context.hpp"

using namespace gw::editor;
using gw::ecs::Entity;

namespace {
constexpr EntityHandle E(std::uint64_t raw) noexcept { return Entity{raw}; }
} // namespace

TEST_CASE("SelectionContext — single select clears previous") {
    SelectionContext sel;
    sel.set(E(1));
    sel.set(E(2));
    CHECK(sel.count() == 1);
    CHECK(sel.primary() == E(2));
    CHECK_FALSE(sel.is_selected(E(1)));
}

TEST_CASE("SelectionContext — toggle adds absent entity") {
    SelectionContext sel;
    sel.toggle(E(10));
    CHECK(sel.count() == 1);
    CHECK(sel.is_selected(E(10)));
}

TEST_CASE("SelectionContext — toggle removes present entity") {
    SelectionContext sel;
    sel.toggle(E(10));
    sel.toggle(E(10));
    CHECK(sel.empty());
}

TEST_CASE("SelectionContext — multi-select via toggle") {
    SelectionContext sel;
    sel.toggle(E(1)); sel.toggle(E(2)); sel.toggle(E(3));
    CHECK(sel.count() == 3);
    CHECK(sel.is_selected(E(1)));
    CHECK(sel.is_selected(E(2)));
    CHECK(sel.is_selected(E(3)));
}

TEST_CASE("SelectionContext — clear empties the selection") {
    SelectionContext sel;
    sel.set(E(5));
    sel.clear();
    CHECK(sel.empty());
    CHECK(sel.primary() == kNullEntity);
}

TEST_CASE("SelectionContext — set(nullEntity) clears") {
    SelectionContext sel;
    sel.set(E(7));
    sel.set(kNullEntity);
    CHECK(sel.empty());
}

TEST_CASE("SelectionContext — on_changed signal fires on set") {
    SelectionContext sel;
    int call_count = 0;
    sel.subscribe([&](const SelectionContext&){ ++call_count; });
    sel.set(E(1));
    sel.set(E(2));
    sel.clear();
    CHECK(call_count == 3);
}

TEST_CASE("SelectionContext — selected() span correct") {
    SelectionContext sel;
    sel.toggle(E(100)); sel.toggle(E(200));
    auto span = sel.selected();
    CHECK(span.size() == 2);
}
