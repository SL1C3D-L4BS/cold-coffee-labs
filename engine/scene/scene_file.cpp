// engine/scene/scene_file.cpp
// `.gwscene` on-disk codec — see scene_file.hpp and ADR-0006.
//
// The codec defers its wire shape entirely to `engine/ecs/serialize.{hpp,cpp}`
// (SnapshotMode::SceneFile). This file is the I/O shell:
//
//   * atomic writes (tmp → rename),
//   * payload flag validation (zstd bit reserved but disabled),
//   * migration registry → ecs::LoadOptions::migrate plumbing.
#include "scene_file.hpp"
#include "migration.hpp"

#include "engine/core/serialization.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <system_error>

namespace gw::scene {

using gw::core::BinaryReader;

std::string_view to_string(SceneIoError e) noexcept {
    switch (e) {
        case SceneIoError::OpenFailed:              return "open failed";
        case SceneIoError::WriteFailed:             return "write failed";
        case SceneIoError::ReadFailed:              return "read failed";
        case SceneIoError::RenameFailed:            return "rename failed";
        case SceneIoError::EmptyFile:               return "empty scene file";
        case SceneIoError::BadMagic:                return "bad magic";
        case SceneIoError::UnsupportedVersion:      return "unsupported scene version";
        case SceneIoError::CrcMismatch:             return "scene payload CRC mismatch";
        case SceneIoError::Truncated:               return "scene file truncated";
        case SceneIoError::PayloadCorrupt:          return "scene payload structurally invalid";
        case SceneIoError::UnsupportedFlags:        return "scene flags not supported (zstd?)";
        case SceneIoError::MigrationFailed:         return "component migration returned invalid bytes";
        case SceneIoError::UnknownComponentType:    return "unknown component type (strict)";
        case SceneIoError::ComponentSizeMismatch:   return "component size mismatch + no migration";
        case SceneIoError::ComponentNotSerializable:return "component not trivially copyable";
    }
    return "unknown";
}

SceneIoError from_ecs(gw::ecs::SerializationError e) noexcept {
    switch (e) {
        case gw::ecs::SerializationError::BadMagic:                 return SceneIoError::BadMagic;
        case gw::ecs::SerializationError::UnsupportedVersion:       return SceneIoError::UnsupportedVersion;
        case gw::ecs::SerializationError::CrcMismatch:              return SceneIoError::CrcMismatch;
        case gw::ecs::SerializationError::Truncated:                return SceneIoError::Truncated;
        case gw::ecs::SerializationError::UnknownComponentType:     return SceneIoError::UnknownComponentType;
        case gw::ecs::SerializationError::ComponentSizeMismatch:    return SceneIoError::ComponentSizeMismatch;
        case gw::ecs::SerializationError::ComponentNotSerializable: return SceneIoError::ComponentNotSerializable;
        case gw::ecs::SerializationError::PayloadCorrupt:           return SceneIoError::PayloadCorrupt;
    }
    return SceneIoError::PayloadCorrupt;
}

namespace {

// Make an ecs::MigrateComponentFn that consults the process-wide registry.
// We capture the registry pointer so tests can swap in an isolated instance
// by way of `encode_scene`/`decode_scene`.
gw::ecs::MigrateComponentFn bind_registry(const MigrationRegistry* reg) {
    if (!reg) return {};
    return [reg](std::uint64_t stable_hash,
                 std::uint32_t from_size,
                 std::uint32_t to_size,
                 std::span<const std::uint8_t> src)
        -> std::optional<std::vector<std::uint8_t>> {
        const auto* entry = reg->find(stable_hash, from_size);
        if (!entry) return std::nullopt;
        if (entry->to_size != to_size) return std::nullopt;
        std::vector<std::uint8_t> out(to_size, std::uint8_t{0});
        if (!entry->fn(src, std::span<std::uint8_t>{out})) {
            return std::nullopt;
        }
        return out;
    };
}

// Inspect the first 8 bytes (magic + version) to pre-flight the blob before
// handing it to the ECS loader. Keeps zstd-enabled files out of the ECS
// parser since it doesn't yet understand compression.
SceneIoError preflight(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 8) return SceneIoError::Truncated;
    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint16_t flags   = 0;
    std::memcpy(&magic,   bytes.data() + 0, 4);
    std::memcpy(&version, bytes.data() + 4, 2);
    std::memcpy(&flags,   bytes.data() + 6, 2);
    if (magic   != kSceneMagic)         return SceneIoError::BadMagic;
    if (version != kSceneFormatVersion) return SceneIoError::UnsupportedVersion;
    if (flags & kFlagZstdCompressed)    return SceneIoError::UnsupportedFlags;
    return SceneIoError::OpenFailed;  // sentinel "no preflight error" branch —
                                       // callers check against the specific
                                       // codes above, not this value.
}

} // namespace

// ---------------------------------------------------------------------------
// In-memory codec.
// ---------------------------------------------------------------------------

std::expected<std::vector<std::uint8_t>, SceneIoError>
encode_scene(const gw::ecs::World& w, SaveOptions opts) {
    if (opts.compress) {
        // zstd not wired yet (ADR-0006 §2.7). Bail now so callers learn
        // immediately rather than producing a file no loader can eat.
        return std::unexpected(SceneIoError::UnsupportedFlags);
    }
    auto blob = gw::ecs::save_world(w, gw::ecs::SnapshotMode::SceneFile);
    if (blob.empty()) {
        // The only non-I/O empty case is "non-trivially-copyable component
        // registered" — a developer error. Map to PayloadCorrupt so the
        // editor surfaces it rather than silently writing an empty file.
        return std::unexpected(SceneIoError::PayloadCorrupt);
    }
    return blob;
}

std::expected<void, SceneIoError>
decode_scene(std::span<const std::uint8_t> bytes, gw::ecs::World& out,
             LoadSceneOptions opts) {
    if (bytes.empty()) return std::unexpected(SceneIoError::EmptyFile);

    switch (preflight(bytes)) {
        case SceneIoError::BadMagic:           return std::unexpected(SceneIoError::BadMagic);
        case SceneIoError::UnsupportedVersion: return std::unexpected(SceneIoError::UnsupportedVersion);
        case SceneIoError::UnsupportedFlags:   return std::unexpected(SceneIoError::UnsupportedFlags);
        case SceneIoError::Truncated:          return std::unexpected(SceneIoError::Truncated);
        default:                               break;
    }

    gw::ecs::LoadOptions ecs_opts;
    ecs_opts.verify_crc                = opts.verify_crc;
    ecs_opts.strict_unknown_components = opts.strict_unknown_components;
    if (opts.run_migrations) {
        ecs_opts.migrate = bind_registry(&MigrationRegistry::global());
    }

    auto r = gw::ecs::load_world(out, bytes, ecs_opts);
    if (!r) return std::unexpected(from_ecs(r.error()));
    return {};
}

// ---------------------------------------------------------------------------
// On-disk codec.
// ---------------------------------------------------------------------------
namespace {

// Build a collision-unlikely temp path alongside the destination.
std::filesystem::path temp_sibling(const std::filesystem::path& path) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    const auto stem = path.filename().string();
    char buf[24];
    std::snprintf(buf, sizeof(buf), ".%016llx.tmp",
                  static_cast<unsigned long long>(rng()));
    return path.parent_path() / (stem + buf);
}

bool write_all(const std::filesystem::path& p,
               std::span<const std::uint8_t> bytes) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f.good()) return false;
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    f.flush();
    return f.good();
}

bool read_all(const std::filesystem::path& p,
              std::vector<std::uint8_t>& out) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f.good()) return false;
    const auto size = f.tellg();
    if (size < 0) return false;
    out.resize(static_cast<std::size_t>(size));
    if (out.empty()) return true;
    f.seekg(0);
    f.read(reinterpret_cast<char*>(out.data()),
           static_cast<std::streamsize>(out.size()));
    return f.good() || f.eof();
}

} // namespace

std::expected<void, SceneIoError>
save_scene(const std::filesystem::path& path, const gw::ecs::World& w,
           SaveOptions opts) {
    auto encoded = encode_scene(w, opts);
    if (!encoded) return std::unexpected(encoded.error());

    std::error_code ec;
    if (const auto parent = path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        // EC is non-fatal here — a pre-existing directory returns ec==0.
    }

    const auto tmp = temp_sibling(path);
    if (!write_all(tmp, *encoded)) {
        std::filesystem::remove(tmp, ec);
        return std::unexpected(SceneIoError::WriteFailed);
    }

    // std::filesystem::rename is required to be atomic on Windows + POSIX
    // when the src and dst are on the same volume. `temp_sibling` always
    // sits beside `path` so the invariant holds.
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        // Cross-device or locked destination — fall back to remove+rename
        // which is non-atomic but at least makes forward progress.
        std::filesystem::remove(path, ec);
        std::filesystem::rename(tmp, path, ec);
        if (ec) {
            std::filesystem::remove(tmp, ec);
            return std::unexpected(SceneIoError::RenameFailed);
        }
    }
    return {};
}

std::expected<void, SceneIoError>
load_scene(const std::filesystem::path& path, gw::ecs::World& out,
           LoadSceneOptions opts) {
    std::vector<std::uint8_t> bytes;
    if (!read_all(path, bytes)) return std::unexpected(SceneIoError::ReadFailed);
    if (bytes.empty())          return std::unexpected(SceneIoError::EmptyFile);
    return decode_scene(std::span<const std::uint8_t>{bytes}, out, opts);
}

} // namespace gw::scene
