#pragma once
// engine/assets/asset_types.hpp
// Canonical registry of asset type tags, magic numbers, and metadata structs.
// Every asset type that persists to disk MUST register its magic here.

#include <cstdint>
#include <string>

namespace gw::assets {

// ---------------------------------------------------------------------------
// AssetType — runtime discriminant for the asset database.
// Values ≤ 0x00FF are engine-owned; ≥ 0x0100 are game-owned.
// ---------------------------------------------------------------------------
enum class AssetType : uint16_t {
    Unknown = 0x0000,
    Mesh    = 0x0001,  // .gwmesh
    Texture = 0x0002,  // .gwtex
    Material= 0x0003,  // .gwmat  (Phase 17)
    Audio   = 0x0004,  // .gwaudio (Phase 10)
    Scene   = 0x0005,  // .gwscene (Phase 8)
    Shader  = 0x0006,  // .gwspv   (Phase 5 / cook time)
    Script  = 0x0007,  // .gwscript (Phase 8)
    // Game-owned range
    Prefab  = 0x0100,
};

// Binary file magic numbers (4 bytes, little-endian ASCII).
// 0x4753574D = 'MWSG' (little-endian: 'G','W','S','M')
namespace magic {
    static constexpr uint32_t kMesh    = 0x4853574D; // 'MWSH'
    static constexpr uint32_t kTexture = 0x58455447; // 'GTEX'
    static constexpr uint32_t kMat     = 0x54414D47; // 'GMAT'
} // namespace magic

// ---------------------------------------------------------------------------
// AssetPath — virtual path within the VFS (e.g. "assets/meshes/crate.gwmesh")
// Using std::string for paths since they are not on the hot path.
// ---------------------------------------------------------------------------
using AssetPath = std::string;

// Cook key — xxHash3-128 stored as two uint64s.
struct CookKey {
    uint64_t hi = 0;
    uint64_t lo = 0;
    bool operator==(const CookKey& o) const noexcept {
        return hi == o.hi && lo == o.lo;
    }
    bool operator!=(const CookKey& o) const noexcept { return !(*this == o); }
};

// Platform identifier for cook-key discrimination.
enum class CookPlatform : uint8_t {
    Win64   = 0,
    LinuxX64= 1,
    Host    = 0xFF, // resolved to the current build target at cook time
};

// Quality/config preset.
enum class CookConfig : uint8_t {
    Debug   = 0,
    Release = 1,
};

} // namespace gw::assets

// std::hash specialisation so CookKey can be used as unordered_map key.
namespace std {
template<>
struct hash<gw::assets::CookKey> {
    std::size_t operator()(const gw::assets::CookKey& k) const noexcept {
        return static_cast<std::size_t>(k.hi ^ (k.lo * 6364136223846793005ULL + 1442695040888963407ULL));
    }
};
} // namespace std
