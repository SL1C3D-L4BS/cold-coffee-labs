#pragma once
// engine/assets/cook/cook_manifest.hpp
// Per-asset cook metadata written by gw_cook and read by the asset database.

#include "../asset_types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace gw::assets::cook {

// Per-asset entry recorded in the cook manifest JSON.
struct CookEntry {
    AssetPath           source;           // e.g. "content/meshes/crate.glb"
    AssetPath           output;           // e.g. "assets/meshes/crate.gwmesh"
    CookKey             cook_key;
    CookKey             source_hash;
    std::vector<AssetPath> deps;          // cooked dependencies
    uint32_t            cook_time_ms = 0;
    bool                cache_hit    = false;
};

// Top-level manifest structure, serialised to JSON by gw_cook.
struct CookManifest {
    uint32_t     cook_version  = 1;
    std::string  platform;               // "win64" | "linux-x64"
    std::string  config;                 // "debug" | "release"
    int64_t      timestamp     = 0;      // unix seconds
    std::string  engine_version;         // GW_VERSION string

    std::vector<CookEntry> assets;

    // Serialise to JSON string.  Implemented in cook_manifest.cpp.
    [[nodiscard]] std::string to_json() const;

    // Write to a file path.  Returns false on IO error.
    bool write_file(const std::string& path) const;
};

// Rule-version constant per cooker type.
// Bump this in the cooker's .cpp when the transform logic changes; that
// invalidates the cache for every asset of that type.
namespace rule_version {
    static constexpr uint32_t kMesh    = 1u;
    static constexpr uint32_t kTexture = 1u;
}

} // namespace gw::assets::cook
