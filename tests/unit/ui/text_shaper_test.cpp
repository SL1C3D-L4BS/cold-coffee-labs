// tests/unit/ui/text_shaper_test.cpp — Phase 11 Wave 11D (ADR-0028).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/ui/font_library.hpp"
#include "engine/ui/text_shaper.hpp"

#include <vector>

using namespace gw::ui;

TEST_CASE("TextShaper: one glyph per ASCII codepoint") {
    FontLibrary fonts;
    auto f = fonts.register_face({"Inter", "Regular", "", 400, false, 1.2f});
    TextShaper s;
    ShapeInput in;
    in.face = f;
    in.utf8 = "Hi";
    in.pixel_size = 16;
    std::vector<ShapedGlyph> out;
    auto n = s.shape(fonts, in, out);
    CHECK(n == 2);
    CHECK(out[0].glyph_index == static_cast<std::uint32_t>('H'));
    CHECK(out[1].glyph_index == static_cast<std::uint32_t>('i'));
    CHECK(out[0].x_advance_26_6 > 0);
}

TEST_CASE("TextShaper: script detection picks CJK over Latin") {
    CHECK(TextShaper::detect_script("hello")         == TextScript::Latin);
    CHECK(TextShaper::detect_script("\xe4\xbd\xa0\xe5\xa5\xbd")     == TextScript::HanS);    // 你好
    CHECK(TextShaper::detect_script("\xd8\xa7\xd9\x84")             == TextScript::Arabic);  // ال
}

TEST_CASE("TextShaper: RTL direction reverses glyph order") {
    FontLibrary fonts;
    auto f = fonts.register_face({"Inter", "Regular", "", 400, false, 1.2f});
    TextShaper s;
    ShapeInput in;
    in.face = f;
    in.utf8 = "ABC";
    in.direction = TextDirection::RTL;
    std::vector<ShapedGlyph> out;
    (void)s.shape(fonts, in, out);
    REQUIRE(out.size() == 3);
    CHECK(out[0].glyph_index == 'C');
    CHECK(out[2].glyph_index == 'A');
}

TEST_CASE("TextShaper: direction_for picks RTL for Arabic/Hebrew") {
    CHECK(TextShaper::direction_for(TextScript::Arabic) == TextDirection::RTL);
    CHECK(TextShaper::direction_for(TextScript::Hebrew) == TextDirection::RTL);
    CHECK(TextShaper::direction_for(TextScript::Latin)  == TextDirection::LTR);
}
