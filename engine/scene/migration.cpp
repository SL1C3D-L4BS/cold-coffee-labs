// engine/scene/migration.cpp
#include "migration.hpp"

namespace gw::scene {

void MigrationRegistry::register_raw(std::uint64_t stable_hash,
                                     std::uint32_t from_size,
                                     std::uint32_t to_size,
                                     MigrateFn     fn,
                                     std::string   description) {
    std::lock_guard lock{mu_};
    MigrationKey key{stable_hash, from_size};
    table_[key] = MigrationEntry{to_size, std::move(fn), std::move(description)};
}

const MigrationEntry*
MigrationRegistry::find(std::uint64_t stable_hash,
                        std::uint32_t from_size) const {
    std::lock_guard lock{mu_};
    auto it = table_.find({stable_hash, from_size});
    return it == table_.end() ? nullptr : &it->second;
}

std::size_t MigrationRegistry::size() const {
    std::lock_guard lock{mu_};
    return table_.size();
}

void MigrationRegistry::clear() {
    std::lock_guard lock{mu_};
    table_.clear();
}

MigrationRegistry& MigrationRegistry::global() {
    static MigrationRegistry instance;
    return instance;
}

} // namespace gw::scene
