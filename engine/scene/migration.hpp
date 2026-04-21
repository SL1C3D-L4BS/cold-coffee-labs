#pragma once
// engine/scene/migration.hpp
// Component schema migration registry — Phase 8 week 043.
//
// ADR-0006 §2.8 allows components to evolve over time. The wire format
// records a stable_hash per component type and (for trivially-copyable
// components) a byte size. When a loader sees a byte size that doesn't
// match the live type's size, the default codec returns
// ComponentSizeMismatch. This registry is the escape hatch:
//
//   MigrationRegistry::global().register_fn<MyComponent>(
//       /*from_size=*/ 12,  // old footprint (e.g. 3x float32 position)
//       [](std::span<const std::uint8_t> src,
//          std::span<std::uint8_t>       dst) {
//           // Rehydrate old bytes into the new layout.
//       });
//
// The registry is queried by scene_file.cpp before it hands the payload to
// ecs::load_world. Callers register migrations once at editor / game
// startup; the registry is thread-safe for reads after the last
// register_fn() call has returned.

#include "engine/ecs/component_registry.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace gw::scene {

// A migration converts a source byte range (the on-disk layout for
// `from_size` bytes) into a destination byte range (the live layout for
// `to_size = sizeof(T)` bytes). Return false to abort the load with
// MigrationFailed.
using MigrateFn =
    std::function<bool(std::span<const std::uint8_t> src,
                       std::span<std::uint8_t>       dst)>;

// Key: (stable_hash, from_size).  Two migrations for the same key collide
// and the later register_fn() overrides the earlier — intentional so
// reloading editor plugins can hot-patch a broken migration.
struct MigrationKey {
    std::uint64_t stable_hash = 0;
    std::uint32_t from_size   = 0;

    friend bool operator==(const MigrationKey& a, const MigrationKey& b) noexcept {
        return a.stable_hash == b.stable_hash && a.from_size == b.from_size;
    }
};

struct MigrationKeyHash {
    std::size_t operator()(const MigrationKey& k) const noexcept {
        // xorshift mix — keeps collisions low across
        // sequential hash values (stable_hash) paired with the handful
        // of distinct from_size values we expect.
        std::uint64_t h = k.stable_hash ^ (std::uint64_t{k.from_size} * 0x9E3779B97F4A7C15ULL);
        h ^= h >> 33;
        h *= 0xFF51AFD7ED558CCDULL;
        h ^= h >> 33;
        return static_cast<std::size_t>(h);
    }
};

struct MigrationEntry {
    std::uint32_t to_size = 0;
    MigrateFn     fn;
    // Optional human-readable tag; written to logs on apply.
    std::string   description;
};

class MigrationRegistry {
public:
    MigrationRegistry() = default;

    MigrationRegistry(const MigrationRegistry&)            = delete;
    MigrationRegistry& operator=(const MigrationRegistry&) = delete;

    // Typed registration: deduces the stable_hash from the live registry's
    // per-type metadata. This is the preferred call site for editor /
    // engine code that already has the component type in scope.
    template <typename T>
    void register_fn(std::uint32_t from_size, MigrateFn fn,
                     std::string description = {}) {
        const auto info = gw::ecs::detail::make_info_for<T>();
        register_raw(info.stable_hash, from_size,
                     static_cast<std::uint32_t>(info.size),
                     std::move(fn), std::move(description));
    }

    // Raw registration — used by plugins that only know stable_hash at
    // runtime (BLD, future save-format tooling).
    void register_raw(std::uint64_t stable_hash,
                      std::uint32_t from_size,
                      std::uint32_t to_size,
                      MigrateFn     fn,
                      std::string   description = {});

    // Lookup. Returns nullptr if the (hash, from_size) pair has no
    // migration registered. Thread-safe for concurrent readers (and
    // concurrent writers — it holds a mutex).
    [[nodiscard]] const MigrationEntry*
    find(std::uint64_t stable_hash, std::uint32_t from_size) const;

    [[nodiscard]] std::size_t size() const;

    // Clear — mostly for tests. Unregisters every migration.
    void clear();

    // Process-wide registry. The default MigrationRegistry for engine +
    // editor lives here so editor startup and tests can share state
    // without a DI handshake.
    [[nodiscard]] static MigrationRegistry& global();

private:
    mutable std::mutex                                                      mu_;
    std::unordered_map<MigrationKey, MigrationEntry, MigrationKeyHash>      table_;
};

} // namespace gw::scene
