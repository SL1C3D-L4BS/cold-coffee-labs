#pragma once
// engine/world/universe/determinism_validator.hpp
// Cross-platform, audit trail for Hell Seed *proving hashes* (Blacklake / Phase 19-C).

#include "engine/core/span.hpp"
#include "engine/core/string.hpp"
#include "engine/world/resources/resource_types.hpp"
#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/universe/hec.hpp"

#include <cstdint>

namespace gw::universe {

struct DeterminismValidator {
    /// MurmurHash3-128 folded to 64 bits: **32-byte Σ_chunk** (raw HEC key) then f32 height lattice
    /// (IEEE754 LE) then u8 biomes. The HEC prefix is the *Hell Seed proof* — it always varies with
    /// the parent universe key even if a stratum (e.g. SDR) is numerically flat in a local box.
    [[nodiscard]] static std::uint64_t hash_chunk(const gw::world::streaming::HeightmapChunk& chunk) noexcept;
    /// Deterministic little-endian struct packing; includes full span length in the key.
    [[nodiscard]] static std::uint64_t hash_resource_nodes(
        gw::core::Span<const gw::world::resources::ResourceNode> nodes) noexcept;
    /// Re-synthesise chunk from Σ₀ + coord (same as `UniverseSeedManager` + HEC(Chunk) path) and test hash.
    [[nodiscard]] static bool validate_cross_platform_stability(const UniverseSeed&     universe_root,
                                                                gw::world::streaming::ChunkCoord              coord,
                                                                std::uint64_t                                expected_hash) noexcept;
    /// One-line per coord JSON: `{ "seed": "<hex of Σ0>", "vectors": [ { "cx", "cy", "cz", "chunk_hash" } ... ] }`
    [[nodiscard]] static gw::core::String compile_hash_report(const UniverseSeed&                       universe_root,
                                                                 gw::core::Span<const gw::world::streaming::ChunkCoord> coords) noexcept;
};

/// @attention **Hell Seed proof vectors (Murmur3)**
///
/// `GoldenHashes` holds the *canonical* 64-bit folded digests of `hash_chunk(HeightmapChunk)` (see
/// `DeterminismValidator::hash_chunk` — 32-byte Σ_child ∥ f32 heights ∥ u8 biomes) for the fixed Σ₀/arena
/// strings below.  When a player or streamer **shares a Hell Seed**, they are sharing a universe that must
/// reproduce these **exact** digests on any Greywater/Clang little-endian build.  Any change to the HEC, SDR,
/// or lattice pack layout **is** a breaking change: re-run
/// `cmake -DGW_INFINITE_SEED_GOLDEN_PROBE=ON` on `apps/sandbox_infinite_seed` (one-shot) and re-lock, then
/// document in *Sacrilege* comms.
namespace GoldenHashes {
    /// HELL_SEED_V1, Σ₀ = `UniverseSeed("HELL_SEED_V1")`, chunk (0,0,0) — re-run
    /// `cmake -DGW_INFINITE_SEED_GOLDEN_PROBE=ON` + `sandbox_infinite_seed` and re-lock if the Murmur3 proof
    /// changes (intentional SDR/HEC regression; coordinate with public Hell Seed comms).
    inline constexpr std::uint64_t HELL_SEED_CIRCLE_1_CHUNK_000 = 0xE040A08B66EAA0D9ULL;
    /// `UniverseSeed("TEST_SEED")` @ (0,0,0) — used by `world/determinism_test.cpp` CI gate.
    inline constexpr std::uint64_t TEST_SEED_CHUNK_000 = 0x17259818AB8602F9ULL;
} // namespace GoldenHashes

} // namespace gw::universe
