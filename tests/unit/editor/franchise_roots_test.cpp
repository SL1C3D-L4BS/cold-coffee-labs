// tests/unit/editor/franchise_roots_test.cpp — Sacrilege franchise cold-start roots.

#include "editor/shell/franchise_roots.hpp"

#include <doctest/doctest.h>

#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;

TEST_CASE("editor/shell/franchise_roots: discover repo and list franchise games") {
    const auto root = gw::editor::shell::find_greywater_repo_root();
    REQUIRE_MESSAGE(root.has_value(),
        "Expected Greywater repo from test working directory (build dir inside tree).");

    REQUIRE(fs::is_regular_file(*root / "franchises" / "sacrilege" / "games.manifest"));

    const auto games = gw::editor::shell::list_sacrilege_franchise_games(*root);
    REQUIRE(games.size() >= 4);

    std::set<std::string> slugs;
    for (const auto& g : games) {
        CHECK(!g.slug.empty());
        CHECK(!g.display_name.empty());
        CHECK(fs::is_directory(g.root));
        slugs.insert(g.slug);
    }
    CHECK(slugs.count("sacrilege") == 1);
    CHECK(slugs.count("apostasy") == 1);
    CHECK(slugs.count("silentium") == 1);
    CHECK(slugs.count("witness") == 1);
}
