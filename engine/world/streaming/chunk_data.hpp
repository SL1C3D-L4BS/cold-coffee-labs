#pragma once
// engine/world/streaming/chunk_data.hpp — Phase 19-B chunk views (arena-backed spans, no owning arrays).

#include "engine/core/span.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/universe/hec.hpp"

#include <cstdint>

namespace gw::world::streaming {

enum class ChunkState : std::uint8_t {
    Unloaded  = 0,
    Queued    = 1,
    Generating = 2,
    Ready     = 3,
    Evicting  = 4,
};

/// Metadata for one streamed volume; height/biome samples live in an external arena slab (views only).
struct ChunkData {
    ChunkCoord                 coord{};
    ChunkState                 state{ChunkState::Unloaded};
    gw::universe::UniverseSeed seed{};
    std::uint64_t              generation_frame{0};
};

inline constexpr std::size_t kHeightmapResolution = 32;
inline constexpr std::size_t kHeightmapSampleCount = kHeightmapResolution * kHeightmapResolution;

/// One Sacrilege circle / special volume per id — see `docs/07_SACRILEGE.md` (Nine Circles mapping).
enum class BiomeId : std::uint8_t {
    Void            = 0,
    LimboIsland     = 1,
    FrozenCavern    = 2,
    FleshTunnel     = 3,
    IndustrialForge = 4,
    Battlefield     = 5,
    HereticLibrary  = 6,
    Arena           = 7,
    DeceptiveMaze   = 8,
    AbyssVoid       = 9,
    COUNT           = 10,
};

/// Height + biome classification for one chunk; `heights` / `biome_ids` are non-owning spans into arena storage.
struct HeightmapChunk : ChunkData {
    gw::core::Span<float>            heights{};
    gw::core::Span<std::uint8_t>    biome_ids{};
};

} // namespace gw::world::streaming
