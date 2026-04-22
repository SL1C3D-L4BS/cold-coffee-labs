#pragma once
// engine/ecs/serialize.hpp
// ECS World <-> binary blob. See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.9.
//
// Phase-7 scope (this file):
//   * Trivially-copyable fast path (ADR §2.4) is implemented end-to-end.
//   * Non-trivially-copyable components are rejected at save time with a
//     ComponentNotSerializable error — no silent data loss. The reflection
//     slow path (ADR §2.5) is the Phase-8 follow-up.
//   * SnapshotMode::SceneFile is accepted but zstd compression is disabled
//     unconditionally (ADR §2.7 says zstd is optional; compressor integration
//     lands with the scene-file asset entry in Phase 8).
//
// All ECS components currently registered by the editor (Name/Transform/
// Visibility/Hierarchy) are trivially copyable, so this is sufficient for
// PIE snapshot/restore and DestroyEntity undo today.

#include "entity.hpp"
#include "world.hpp"

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace gw::ecs {

// ---------------------------------------------------------------------------
// Framing mode — identical payload, different wrapping.
// ---------------------------------------------------------------------------
enum class SnapshotMode : std::uint8_t {
    PieSnapshot,     // header, no compression
    SceneFile,       // header, compression flag reserved for Phase 8
    EntitySnapshot,  // no header (inside a command)
};

// ---------------------------------------------------------------------------
// Errors (ADR §2.10).
// ---------------------------------------------------------------------------
enum class SerializationError : std::uint8_t {
    BadMagic,
    UnsupportedVersion,
    CrcMismatch,
    Truncated,
    UnknownComponentType,
    ComponentSizeMismatch,
    ComponentNotSerializable,  // non-trivially-copyable; reflection path pending
    PayloadCorrupt,
};

[[nodiscard]] std::string_view to_string(SerializationError error) noexcept;

// Migration callback — invoked when a component's on-disk size does not
// match the live type's size (ADR-0006 §2.8). The callback returns the
// migrated byte sequence (must equal `to_size`) or std::nullopt to reject
// the load with ComponentSizeMismatch. See engine/scene/migration.hpp for
// the scene-level registry that wraps this hook.
using MigrateComponentFn = std::function<std::optional<std::vector<std::uint8_t>>(
    std::uint64_t                 stable_hash,
    std::uint32_t                 from_size,
    std::uint32_t                 to_size,
    std::span<const std::uint8_t> src)>;

struct LoadOptions {
    bool                 verify_crc                = true;
    bool                 strict_unknown_components = false;
    MigrateComponentFn   migrate;
};

// ---------------------------------------------------------------------------
// World-level API.
// ---------------------------------------------------------------------------

// Returns an empty vector on failure. Failure is rare and upstream of the
// ADR-specified error surface (e.g. a component registered but with no copy
// semantics); callers treat empty as an internal bug.
[[nodiscard]] std::vector<std::uint8_t>
save_world(const World& world, SnapshotMode mode);

[[nodiscard]] std::expected<void, SerializationError>
load_world(World&                target_world,
             std::span<const std::uint8_t> blob,
             const LoadOptions&    opts = {});

// ---------------------------------------------------------------------------
// Single-entity paths (used by DestroyEntityCommand — ADR-0005 §2.7 / §2.6).
// EntitySnapshot mode; no header. The save path refuses dead entities.
// ---------------------------------------------------------------------------
[[nodiscard]] std::vector<std::uint8_t>
save_entity(const World& world, Entity entity);

// Returns a fresh entity handle (generation advanced) populated with the
// components encoded in `blob`. The old entity_bits recorded in the blob are
// ignored for the live handle; callers wanting old→new remapping keep the
// previous handle externally (DestroyEntityCommand does this).
[[nodiscard]] std::expected<Entity, SerializationError>
load_entity(World& world, std::span<const std::uint8_t> blob);

} // namespace gw::ecs
