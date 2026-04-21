// tests/unit/ui/glyph_atlas_test.cpp — Phase 11 Wave 11D (ADR-0028).

#include <doctest/doctest.h>

#include "engine/ui/glyph_atlas.hpp"

#include <vector>

using namespace gw::ui;

namespace {
FontFaceId face(std::uint32_t v) { return FontFaceId{v}; }
}

TEST_CASE("GlyphAtlas: bake + lookup returns same rect") {
    GlyphAtlas atlas(256);
    GlyphKey key{face(1), 'A', 16, false};
    std::vector<std::uint8_t> px(16 * 16, 0xFFu);
    auto rect = atlas.bake(key, 16, 16, std::span<const std::uint8_t>{px}, 0, 0, 0);
    CHECK(rect.w == 16);
    CHECK(rect.h == 16);
    const auto* got = atlas.lookup(key);
    REQUIRE(got != nullptr);
    CHECK(got->x == rect.x);
    CHECK(got->y == rect.y);
}

TEST_CASE("GlyphAtlas: oversize glyph returns empty rect") {
    GlyphAtlas atlas(64);
    GlyphKey key{face(1), 'A', 128, false};
    auto rect = atlas.bake(key, 128, 128, {}, 0, 0, 0);
    CHECK(rect.w == 0);
}

TEST_CASE("GlyphAtlas: colour pages live separately from greyscale pages") {
    GlyphAtlas atlas(128);
    std::vector<std::uint8_t> grey(16 * 16, 0x80u);
    std::vector<std::uint8_t> bgra(16 * 16 * 4, 0xCCu);
    (void)atlas.bake({face(1), 1, 16, false}, 16, 16, std::span<const std::uint8_t>{grey}, 0, 0, 0);
    (void)atlas.bake({face(1), 2, 16, true},  16, 16, std::span<const std::uint8_t>{bgra}, 0, 0, 0);
    CHECK(atlas.page_count() == 2);
    bool seen_color = false, seen_grey = false;
    for (std::size_t i = 0; i < atlas.page_count(); ++i) {
        if (atlas.page(i).color) seen_color = true; else seen_grey = true;
    }
    CHECK(seen_color);
    CHECK(seen_grey);
}

TEST_CASE("GlyphAtlas: wraps to next row when page runs out horizontally") {
    GlyphAtlas atlas(32);
    std::vector<std::uint8_t> px(8 * 8, 0xAA);
    auto r0 = atlas.bake({face(1), 1, 8, false}, 8, 8, std::span<const std::uint8_t>{px}, 0, 0, 0);
    (void)atlas.bake({face(1), 2, 8, false}, 8, 8, std::span<const std::uint8_t>{px}, 0, 0, 0);
    (void)atlas.bake({face(1), 3, 8, false}, 8, 8, std::span<const std::uint8_t>{px}, 0, 0, 0);
    auto r3 = atlas.bake({face(1), 4, 8, false}, 8, 8, std::span<const std::uint8_t>{px}, 0, 0, 0);
    // First 4 fit on row 0 for a 32px page, but 8*4 = 32 exactly; the fifth
    // must wrap. Add one more to observe wrap.
    auto r4 = atlas.bake({face(1), 5, 8, false}, 8, 8, std::span<const std::uint8_t>{px}, 0, 0, 0);
    CHECK(r0.y == 0);
    CHECK(r3.y == 0);
    CHECK(r4.y > 0);
    CHECK(atlas.glyph_count() == 5);
}
