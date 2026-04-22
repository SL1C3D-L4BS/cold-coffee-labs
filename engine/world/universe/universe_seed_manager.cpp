#include "engine/world/universe/universe_seed_manager.hpp"

namespace gw::universe {

void UniverseSeedManager::set_seed(const UniverseSeed& seed) noexcept {
    const std::unique_lock lk(mutex_);
    active_ = seed;
    ++cache_epoch_;
}

UniverseSeed UniverseSeedManager::get_active_seed() const noexcept {
    const std::shared_lock lk(mutex_);
    return active_;
}

UniverseSeed UniverseSeedManager::get_chunk_seed(const std::int64_t cx, const std::int64_t cy,
                                                 const std::int64_t cz) const noexcept {
    UniverseSeed parent{};
    {
        const std::shared_lock lk(mutex_);
        parent = active_;
    }
    return hec_derive(parent, HecDomain::Chunk, cx, cy, cz);
}

UniverseSeed UniverseSeedManager::get_arena_seed(const std::int32_t circle_index) const noexcept {
    UniverseSeed parent{};
    {
        const std::shared_lock lk(mutex_);
        parent = active_;
    }
    return hec_derive(parent, HecDomain::Arena, static_cast<std::int64_t>(circle_index), 0, 0);
}

std::uint64_t UniverseSeedManager::cache_epoch() const noexcept {
    const std::shared_lock lk(mutex_);
    return cache_epoch_;
}

} // namespace gw::universe
