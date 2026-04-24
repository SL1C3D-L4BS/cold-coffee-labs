// tests/unit/play/playable_paths_test.cpp — PIE / sandbox_playable path parity.

#include <doctest/doctest.h>

#include "engine/play/playable_paths.hpp"

TEST_CASE("play_cvars_rel_for_scene_path — posix scene") {
    CHECK(gw::play::play_cvars_rel_for_scene_path("content/scenes/demo.gwscene") ==
          "content/scenes/demo.play_cvars.toml");
}

TEST_CASE("play_cvars_rel_for_scene_path — no directory component") {
    CHECK(gw::play::play_cvars_rel_for_scene_path("demo.gwscene") ==
          "demo.play_cvars.toml");
}
