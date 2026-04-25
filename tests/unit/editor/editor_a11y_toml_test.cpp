// tests/unit/editor/editor_a11y_toml_test.cpp — Wave 1C a11y TOML round-trip.

#include <doctest/doctest.h>

#include "editor/a11y/editor_a11y.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace {

std::filesystem::path unique_temp_a11y_path() {
    static std::uint32_t     seq = 0;
    const std::string        name = "gw_editor_a11y_roundtrip_" + std::to_string(++seq) + ".toml";
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST_CASE("EditorA11yConfig TOML save/load round-trips all seven bools") {
    const std::filesystem::path p  = unique_temp_a11y_path();
    std::error_code              ec{};

    gw::editor::a11y::EditorA11yConfig original{};
    original.reduce_corruption   = true;
    original.disable_vignette     = true;
    original.force_high_contrast  = false;
    original.force_mono_font      = true;
    original.keyboard_only_nav   = false;
    original.colour_blind_wong  = true;
    original.reduce_motion        = true;

    gw::editor::a11y::save_to_path(original, p);
    const gw::editor::a11y::EditorA11yConfig round = gw::editor::a11y::load_from_path(p);
    std::filesystem::remove(p, ec);

    CHECK(round.reduce_corruption == original.reduce_corruption);
    CHECK(round.disable_vignette == original.disable_vignette);
    CHECK(round.force_high_contrast == original.force_high_contrast);
    CHECK(round.force_mono_font == original.force_mono_font);
    CHECK(round.keyboard_only_nav == original.keyboard_only_nav);
    CHECK(round.colour_blind_wong == original.colour_blind_wong);
    CHECK(round.reduce_motion == original.reduce_motion);
}

TEST_CASE("EditorA11yConfig TOML missing file returns defaults") {
    const std::filesystem::path p  = unique_temp_a11y_path();
    const auto                   got = gw::editor::a11y::load_from_path(p);
    (void)std::filesystem::remove(p);

    const gw::editor::a11y::EditorA11yConfig d{};
    CHECK(got.reduce_corruption == d.reduce_corruption);
    CHECK(got.disable_vignette == d.disable_vignette);
    CHECK(got.force_high_contrast == d.force_high_contrast);
    CHECK(got.force_mono_font == d.force_mono_font);
    CHECK(got.keyboard_only_nav == d.keyboard_only_nav);
    CHECK(got.colour_blind_wong == d.colour_blind_wong);
    CHECK(got.reduce_motion == d.reduce_motion);
}
