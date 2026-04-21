#pragma once
// engine/scene/scene_file.hpp
// `.gwscene` on-disk codec — Phase 8 week 043.
//
// The wire format is defined by ADR-0006 §2 ("ECS world serialization"). This
// layer is the file shell over it:
//
//   * save_scene()  writes the GWS1 header + payload produced by
//                   ecs::save_world(SceneFile) to `path`, using an atomic
//                   write-then-rename so a crash never leaves a half-written
//                   `.gwscene` beside a player's save.
//   * load_scene()  reads the bytes, runs them through the migration
//                   registry (see migration.hpp), and invokes
//                   ecs::load_world() into the caller's world.
//
// zstd compression is reserved per ADR-0006 §2.7 (flags bit 0) but not yet
// enabled; `save_scene` always emits an uncompressed payload. `load_scene`
// rejects a blob whose flags claim compression because we have no
// decompressor linked yet. When zstd lands, the flag bit flips to "on" and
// only the codec changes — the header + migration layers stay byte-identical.

#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace gw::scene {

// File-level errors. Superset of ecs::SerializationError so the BLD API
// surface can forward a single enum downstream.
enum class SceneIoError : std::uint8_t {
    // I/O
    OpenFailed,
    WriteFailed,
    ReadFailed,
    RenameFailed,
    EmptyFile,
    // Format
    BadMagic,
    UnsupportedVersion,
    CrcMismatch,
    Truncated,
    PayloadCorrupt,
    UnsupportedFlags,        // e.g. zstd bit set but no decompressor
    MigrationFailed,
    // Downstream
    UnknownComponentType,
    ComponentSizeMismatch,
    ComponentNotSerializable,
};

[[nodiscard]] std::string_view to_string(SceneIoError e) noexcept;

// Map an ecs::SerializationError onto a SceneIoError. Kept here rather than
// in the ECS layer because the mapping is a file-shell concern.
[[nodiscard]] SceneIoError from_ecs(gw::ecs::SerializationError e) noexcept;

struct SaveOptions {
    // Reserved for zstd (ADR-0006 §2.7). Setting this today returns
    // UnsupportedFlags until the decompressor lands.
    bool compress = false;
};

struct LoadSceneOptions {
    bool verify_crc                = true;
    bool strict_unknown_components = false;
    // When true, run the migration registry (migration.hpp) before applying
    // the payload. Defaults to on — schema evolution is a first-class path.
    bool run_migrations            = true;
};

// ----- File I/O ------------------------------------------------------------

// Atomic write: serialises `w`, writes to `<path>.tmp`, fsyncs, then renames
// over `path`. Returns OpenFailed / WriteFailed / RenameFailed on first
// failure. Creates parent directories if needed.
[[nodiscard]] std::expected<void, SceneIoError>
save_scene(const std::filesystem::path& path, const gw::ecs::World& w,
           SaveOptions opts = {});

// Reads `path` and populates `out` (which is cleared first). `out`'s
// ComponentRegistry must already have the relevant component types
// registered — the loader resolves types by ADR-0006 stable_hash.
[[nodiscard]] std::expected<void, SceneIoError>
load_scene(const std::filesystem::path& path, gw::ecs::World& out,
           LoadSceneOptions opts = {});

// In-memory variants, primarily for tests. Both share the on-disk codec.
[[nodiscard]] std::expected<std::vector<std::uint8_t>, SceneIoError>
encode_scene(const gw::ecs::World& w, SaveOptions opts = {});

[[nodiscard]] std::expected<void, SceneIoError>
decode_scene(std::span<const std::uint8_t> bytes, gw::ecs::World& out,
             LoadSceneOptions opts = {});

// Format constants, mirrored from engine/ecs/serialize.cpp for consumers
// that need to inspect bytes without linking ecs/serialize.cpp.
inline constexpr std::uint32_t kSceneMagic         = 0x31535747u;  // 'GWS1'
inline constexpr std::uint16_t kSceneFormatVersion = 1u;
inline constexpr std::uint16_t kFlagZstdCompressed = 1u << 0;

} // namespace gw::scene
