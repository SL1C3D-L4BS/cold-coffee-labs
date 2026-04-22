#pragma once
// engine/persist/persist_types.hpp — Phase 15 (ADR-0056..0057).

#include <cstdint>
#include <string>
#include <string_view>

#include <glm/glm.hpp>

namespace gw::persist {

inline constexpr std::uint32_t kCurrentSaveSchemaVersion = 2;

using SlotId = std::int32_t;

enum class SaveError : std::uint8_t {
    Ok = 0,
    IoError,
    BadPath,
    SlotMissing,
    CloudError,
};

enum class LoadError : std::uint8_t {
    Ok = 0,
    BadMagic,
    TruncatedContainer,
    UnsupportedVersion,
    UnsupportedCompression,
    ForwardIncompatible,
    IntegrityMismatch,
    DeterminismMismatch,
    MigrationGap,
    SchemaMismatch,
    IoError,
};

[[nodiscard]] std::string_view to_string(LoadError e) noexcept;
[[nodiscard]] std::string_view to_string(SaveError e) noexcept;

struct SaveMeta {
    std::string           display_name{};
    std::uint64_t         world_seed{0};
    std::uint64_t         session_seed{0};
    std::uint64_t         created_unix_ms{0};
    glm::dvec3            anchor_pos{0.0};
    std::uint32_t         schema_version{kCurrentSaveSchemaVersion};
    std::uint64_t         determinism_hash{0};
    /// Copy of consent tier string at save time (ADR-0062).
    std::string           consent_tier_snapshot{};
};

[[nodiscard]] std::uint32_t pack_engine_semver(int major, int minor, int patch) noexcept;

} // namespace gw::persist
