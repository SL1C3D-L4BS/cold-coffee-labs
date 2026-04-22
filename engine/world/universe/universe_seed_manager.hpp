#pragma once

#include "engine/world/universe/hec.hpp"

#include <cstdint>
#include <shared_mutex>

namespace gw::universe {

/// Owns the active "Hell Seed" Σ₀ for a Sacrilege session — docs/08 §1.1 master entropy register.
/// Injected into `WorldContext` (explicit ownership); not a hidden global.
class UniverseSeedManager {
public:
    UniverseSeedManager() noexcept = default;

    void set_seed(const UniverseSeed& seed) noexcept;

    [[nodiscard]] UniverseSeed get_active_seed() const noexcept;

    [[nodiscard]] UniverseSeed get_chunk_seed(std::int64_t cx, std::int64_t cy, std::int64_t cz) const noexcept;

    [[nodiscard]] UniverseSeed get_arena_seed(std::int32_t circle_index) const noexcept;

    /// Monotonic token bumped on every `set_seed()` so streaming caches can detect invalidation cheaply.
    [[nodiscard]] std::uint64_t cache_epoch() const noexcept;

private:
    mutable std::shared_mutex mutex_{};
    UniverseSeed                active_{};
    std::uint64_t               cache_epoch_{0};
};

} // namespace gw::universe
