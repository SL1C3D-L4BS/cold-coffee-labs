// tests/unit/editor/editor_context_test.cpp
// EditorContext aggregate — PIE transport fields default safely for headless tests.
#include <doctest/doctest.h>
#include "editor/panels/panel.hpp"
#include "editor/selection/selection_context.hpp"
#include "editor/undo/command_stack.hpp"

using namespace gw::editor;

TEST_CASE("EditorContext — PIE transport fields default to safe no-op") {
    SelectionContext              sel;
    gw::editor::undo::CommandStack stack;
    EditorContext ctx{.selection = sel, .cmd_stack = stack};
    CHECK_FALSE(ctx.in_pie);
    CHECK_FALSE(ctx.pie_paused);
    CHECK(ctx.pie_pause_toggle == nullptr);
    CHECK(ctx.pie_pause_user_data == nullptr);
}
