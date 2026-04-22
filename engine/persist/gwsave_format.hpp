#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gw::persist::gwsave {

inline constexpr char kMagic[4]       = {'G', 'W', 'S', 'V'};
inline constexpr char kFooterMagic[4] = {'F', 'T', 'R', '!'};
inline constexpr std::uint16_t kContainerVersion = 1;

/// Container `flags` field (little-endian u16). Bit 0: chunk bodies are zstd-compressed.
inline constexpr std::uint16_t kFlagZstd = 1u;

// Through `chunk_count` (exclusive of per-chunk TOC): magic..chunk_count = 76 bytes.
inline constexpr std::size_t kHeaderPrefixBytes = 76;

inline constexpr std::size_t kTocEntryBytes = 12 + 8 + 8 + 8;

struct HeaderPrefix {
    std::uint16_t flags{0};
    std::uint32_t schema_version{0};
    std::uint32_t engine_version{0};
    std::uint64_t world_seed{0};
    std::uint64_t session_seed{0};
    std::uint64_t created_unix_ms{0};
    double        anchor_x{0};
    double        anchor_y{0};
    double        anchor_z{0};
    std::uint64_t determinism_hash{0};
    std::uint32_t chunk_count{0};
};

struct TocEntry {
    std::int32_t  cx{0};
    std::int32_t  cy{0};
    std::int32_t  cz{0};
    std::uint64_t offset{0};
    std::uint64_t length{0};
    std::uint64_t sub_digest{0};
};

struct ChunkPayload {
    std::int32_t              cx{0};
    std::int32_t              cy{0};
    std::int32_t              cz{0};
    std::vector<std::byte>    bytes{};
};

} // namespace gw::persist::gwsave
