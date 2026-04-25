#pragma once

// Phase 18 — Studio Ready milestone.
// Mod surface deliberately limited to World& + AssetDatabase& to prevent mods from
// taking undocumented dependencies on render or job internals. Any capability a mod
// needs that is not expressible through this surface requires a new public API and an ADR.

#include "engine/core/array.hpp"
#include "engine/core/result.hpp"
#include "engine/core/string.hpp"
#include "engine/core/version.hpp"
#include "engine/platform/dll.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>
#include <variant>

namespace gw {
namespace ecs {
class World;
}
namespace assets {
class AssetDatabase;
}
}  // namespace gw

namespace gw {
namespace scripting {

using f32 = float;

using Path = std::filesystem::path;

enum class ModError : std::uint8_t {
    None = 0,
    OpenFailed,
    BadManifest,
    VersionMismatch,
    LoadFailed,
    SymbolMissing,
    UnloadFailed,
    RegistryFull,
};

struct ModHandle {
    std::uint32_t value = 0;
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
};

/// `std::monostate` models `void` success (`gw::core::Result` cannot hold `void` as a value type).
using ModLoadResult = gw::core::Result<std::monostate, ModError>;

/// Manifest fields loaded from the mod's `mod_manifest.json`.
struct ModManifest {
    gw::core::String    mod_id{};
    gw::core::String    display_name{};
    gw::version::SemVer version{};
    gw::version::SemVer engine_min_version{};
    Path                gameplay_module_path{};
};

/// Fuzz/CI entry: structural parse of the flat JSON the loader expects (no dylib
/// I/O, no version gate against the running engine).
[[nodiscard]] bool try_parse_mod_manifest_from_json(std::string_view json, ModManifest& out) noexcept;

/// In-host view of a loaded mod. Hot path uses `std::function` (no vtable on the mod side)
/// and is populated from C-ABI entry points when the mod is a dynamic library.
struct IModRuntime {
    std::function<ModLoadResult(gw::ecs::World&, gw::assets::AssetDatabase&)> on_load{};
    std::function<void(gw::ecs::World&)>                                        on_unload{};
    std::function<void(gw::ecs::World&, f32)>                                   on_tick{};
};

/// Owns mod dylibs, wires C exports into `IModRuntime`, and tracks hot-reload.
class ModRegistry {
public:
    static constexpr std::size_t kMaxLoadedMods = 32;

    ModRegistry(gw::ecs::World& world, gw::assets::AssetDatabase& assets) noexcept
        : world_(world)
        , assets_(assets) {}

    ~ModRegistry();

    [[nodiscard]] gw::core::Result<ModHandle, ModError> load_mod(const Path& manifest_path);

    void unload_mod(ModHandle handle);

    void tick_all(gw::ecs::World& world, f32 dt);

private:
    struct LoadedMod {
        bool                         used{false};
        ModHandle                    id{};
        ModManifest                  manifest{};
        IModRuntime                  runtime{};
        gw::platform::DynamicLibrary lib{};
        std::uint64_t                last_mtime{0};
        Path                         manifest_path{};
        Path                         module_path{};
    };

    void poll_hot_reload_(gw::ecs::World& world);
    void unload_slot_(std::size_t slot, gw::ecs::World& world) noexcept;
    [[nodiscard]] std::size_t find_handle_slot_(ModHandle h) const noexcept;
    [[nodiscard]] std::size_t alloc_slot_() const noexcept;

    gw::ecs::World&           world_;
    gw::assets::AssetDatabase& assets_;
    std::uint32_t              next_id_{1};

    using ModArray = gw::core::Array<LoadedMod, kMaxLoadedMods>;
    ModArray mods_{};
};

}  // namespace scripting
}  // namespace gw
