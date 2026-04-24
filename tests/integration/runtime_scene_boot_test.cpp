// tests/integration/runtime_scene_boot_test.cpp — headless runtime + .gwscene + ticks (plan Track A1).

#include <doctest/doctest.h>

#include "editor/scene/authoring_scene_load.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/scene_file.hpp"
#include "runtime/engine.hpp"
#include "runtime/playable_bootstrap.hpp"
#include "runtime/playable_scene_host.hpp"

#include <cstdio>
#include <filesystem>
#include <random>

namespace {

std::filesystem::path unique_scene_path() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    auto dir = std::filesystem::temp_directory_path() / "gw_runtime_scene_boot_tests";
    std::filesystem::create_directories(dir);
    char buf[72];
    std::snprintf(buf, sizeof(buf), "boot_%016llx.gwscene",
                  static_cast<unsigned long long>(rng()));
    return dir / buf;
}

} // namespace

TEST_CASE("runtime — scene load then tick (bootstrap + physics)") {
    gw::ecs::World src;
    gw::editor::scene::ensure_authoring_scene_types_for_load(src);
    const auto e = src.create_entity();
    gw::editor::scene::TransformComponent t{};
    gw::editor::scene::BlockoutPrimitiveComponent b{};
    b.shape = 0;
    src.add_component(e, t);
    src.add_component(e, b);

    const auto path = unique_scene_path();
    REQUIRE(gw::scene::save_scene(path, src).has_value());

    gw::runtime::EngineConfig cfg{};
    cfg.headless      = true;
    cfg.deterministic = true;
    gw::runtime::Engine engine{cfg};

    gw::runtime::PlayableCliOptions cli{};
    cli.universe_seed = 42;
    gw::runtime::apply_playable_bootstrap(engine, cli);

    gw::runtime::PlayableSceneLoadSummary sum{};
    REQUIRE(gw::runtime::load_authoring_scene_into_physics(
        engine, path.generic_string(), sum));
    CHECK(sum.blockout_bodies >= 1);

    engine.run_frames(4);
    CHECK(engine.frame_index() == 4);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}
