// tests/mod/mod_loader_test.cpp — mod registry + hot-reload (Phase 18-C).

#include <doctest/doctest.h>

#include "engine/assets/asset_db.hpp"
#include "engine/assets/vfs/virtual_fs.hpp"
#include "engine/ecs/world.hpp"
#include "engine/platform/fs.hpp"
#include "engine/scripting/mod_api.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#if defined(_WIN32)
#  include <stdlib.h>
#  define MOD_DLL "example_mod.dll"
#else
#  include <stdlib.h>
#  define MOD_DLL "example_mod.so"
#endif

namespace {

[[nodiscard]] std::filesystem::path mods_dir() {
    return std::filesystem::path(GW_MOD_BUILD) / "mods";
}

[[nodiscard]] std::string mods_manifest_path() {
    return (mods_dir() / "mod_manifest.json").string();
}

void set_load_stamp_env(const std::string& path) {
#if defined(_WIN32)
    (void)_putenv_s("GW_MOD_LOAD_STAMP", path.c_str());
#else
    (void)setenv("GW_MOD_LOAD_STAMP", path.c_str(), 1);
#endif
}

void clear_load_stamp_env() {
#if defined(_WIN32)
    (void)_putenv_s("GW_MOD_LOAD_STAMP", "");
#else
    (void)unsetenv("GW_MOD_LOAD_STAMP");
#endif
}

[[nodiscard]] std::size_t stamp_byte_count(const std::filesystem::path& p) {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return 0;
    }
    const auto sz = std::filesystem::file_size(p, ec);
    if (ec) {
        return 0;
    }
    return static_cast<std::size_t>(sz);
}

}  // namespace

TEST_CASE("mod_loader: load example_mod, tick, unload") {
    gw::ecs::World                     w;
    gw::assets::vfs::VirtualFilesystem vfs;
    gw::assets::AssetDatabase          assets{gw::assets::AssetDatabaseModHarnessTag{}, vfs};

    gw::scripting::ModRegistry reg(w, assets);
    const std::string          mpath = mods_manifest_path();
    if (!gw::platform::FileSystem::exists(mpath)) {
        MESSAGE("skip: " << mpath << " not found (build example_mod first)");
        return;
    }
    const auto h = reg.load_mod(mpath);
    REQUIRE(h.has_value());
    for (int i = 0; i < 3; ++i) {
        reg.tick_all(w, 1.f / 60.f);
    }
    reg.unload_mod(h.value());
}

TEST_CASE("mod_loader: version mismatch when engine < engine_min") {
    gw::ecs::World                     w;
    gw::assets::vfs::VirtualFilesystem vfs;
    gw::assets::AssetDatabase          assets{gw::assets::AssetDatabaseModHarnessTag{}, vfs};

    const std::filesystem::path p = std::filesystem::path(GW_MOD_BUILD) / "mod_bad_version.json";
    {
        std::ofstream o(p, std::ios::out | std::ios::trunc);
        o << "{\n";
        o << "  \"mod_id\": \"badver\",\n";
        o << "  \"version\": \"1.0.0\",\n";
        o << "  \"engine_min_version\": \"9.9.9\",\n";
        o << "  \"gameplay_module_path\": \"mods/" << MOD_DLL << "\"\n";
        o << "}\n";
    }
    gw::scripting::ModRegistry reg(w, assets);
    const auto                 r = reg.load_mod(p);
    REQUIRE(!r.has_value());
    CHECK(r.error() == gw::scripting::ModError::VersionMismatch);
    std::error_code ec;
    std::filesystem::remove(p, ec);
}

TEST_CASE("mod_loader: hot-reload calls on_load again after dylib mtime change") {
    const std::filesystem::path dy = mods_dir() / MOD_DLL;
    const std::string           m = mods_manifest_path();
    if (!gw::platform::FileSystem::exists(dy) || !gw::platform::FileSystem::exists(m)) {
        MESSAGE("skip: mod artifacts missing");
        return;
    }
    const std::filesystem::path stamp_path = std::filesystem::path(GW_MOD_BUILD) / "mod_hot_reload_stamp.bin";
    std::error_code             ec;
    std::filesystem::remove(stamp_path, ec);
    set_load_stamp_env(stamp_path.string());

    gw::ecs::World                     w;
    gw::assets::vfs::VirtualFilesystem vfs;
    gw::assets::AssetDatabase          assets{gw::assets::AssetDatabaseModHarnessTag{}, vfs};

    {
        gw::scripting::ModRegistry reg(w, assets);
        REQUIRE(reg.load_mod(m).has_value());
        CHECK(stamp_byte_count(stamp_path) == 1u);
        REQUIRE(gw::platform::FileSystem::bump_last_write_stamp(dy));
        for (int i = 0; i < 8; ++i) {
            reg.tick_all(w, 1.f / 60.f);
            if (stamp_byte_count(stamp_path) >= 2u) {
                break;
            }
        }
        CHECK(stamp_byte_count(stamp_path) >= 2u);
    }

    clear_load_stamp_env();
    std::filesystem::remove(stamp_path, ec);
}
