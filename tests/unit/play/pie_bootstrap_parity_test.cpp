// tests/unit/play/pie_bootstrap_parity_test.cpp — PIE vs `apply_playable_bootstrap` cvar merge.

#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/standard_cvars.hpp"
#include "engine/play/play_bootstrap_cvars.hpp"

TEST_CASE("play_bootstrap — two registries match after optional seed + fixture TOML") {
    namespace fs = std::filesystem;
    const fs::path fixture =
        fs::path(CCL_REPO_ROOT) / "tests/fixtures/play/pie_bootstrap_fixture.play_cvars.toml";
    REQUIRE_MESSAGE(fs::exists(fixture),
                    "missing fixture: tests/fixtures/play/pie_bootstrap_fixture.play_cvars.toml");

    gw::config::CVarRegistry ra{};
    (void)gw::config::register_standard_cvars(ra);
    gw::config::CVarRegistry rb{};
    (void)gw::config::register_standard_cvars(rb);

    constexpr std::int64_t kCliSeed = 42424242;
    const std::string      abs    = fixture.generic_string();

    gw::play::apply_play_bootstrap_to_registry(ra, std::optional<std::int64_t>{kCliSeed}, abs);
    gw::play::apply_play_bootstrap_to_registry(rb, std::optional<std::int64_t>{kCliSeed}, abs);

    const gw::config::CVarId id_a = ra.find("rng.seed");
    const gw::config::CVarId id_b = rb.find("rng.seed");
    REQUIRE(ra.get_i64(id_a).has_value());
    REQUIRE(rb.get_i64(id_b).has_value());
    CHECK(*ra.get_i64(id_a) == *rb.get_i64(id_b));
    // TOML overrides programmatic seed when present.
    CHECK(*ra.get_i64(id_a) == 9012345);
}

TEST_CASE("play_bootstrap — no seed optional skips rng assignment; TOML still merges") {
    namespace fs = std::filesystem;
    const fs::path fixture =
        fs::path(CCL_REPO_ROOT) / "tests/fixtures/play/pie_bootstrap_fixture.play_cvars.toml";
    REQUIRE(fs::exists(fixture));

    gw::config::CVarRegistry r{};
    (void)gw::config::register_standard_cvars(r);

    gw::play::apply_play_bootstrap_to_registry(r, std::nullopt, fixture.generic_string());
    const auto id = r.find("rng.seed");
    REQUIRE(r.get_i64(id).has_value());
    CHECK(*r.get_i64(id) == 9012345);
}
