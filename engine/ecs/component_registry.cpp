// engine/ecs/component_registry.cpp
// See docs/adr/0004-ecs-world.md §2.4.

#include "component_registry.hpp"

#include <stdexcept>
#include <utility>

namespace gw {
namespace ecs {

ComponentRegistry::ComponentRegistry() {
    infos_.reserve(64);
    hash_to_id_.reserve(64);
    lookup_cache_.reserve(64);
}

ComponentRegistry::~ComponentRegistry() = default;

ComponentRegistry::ComponentRegistry(ComponentRegistry&&) noexcept            = default;
ComponentRegistry& ComponentRegistry::operator=(ComponentRegistry&&) noexcept = default;

const ComponentTypeInfo& ComponentRegistry::info(ComponentTypeId id) const {
    if (id >= infos_.size()) {
        throw std::out_of_range("ComponentRegistry::info: invalid ComponentTypeId");
    }
    return infos_[id];
}

std::optional<ComponentTypeId>
ComponentRegistry::lookup_by_hash(std::uint64_t stable_hash) const noexcept {
    auto it = hash_to_id_.find(stable_hash);
    if (it == hash_to_id_.end()) return std::nullopt;
    return it->second;
}

ComponentTypeId ComponentRegistry::register_raw(const ComponentTypeInfo& info) {
    if (auto it = hash_to_id_.find(info.stable_hash); it != hash_to_id_.end()) {
        return it->second;
    }
    if (infos_.size() >= kMaxComponentTypes) {
        throw std::runtime_error(
            "ComponentRegistry: kMaxComponentTypes exceeded; raise the limit "
            "in entity.hpp or stop registering new component types");
    }
    const auto id = static_cast<ComponentTypeId>(infos_.size());
    infos_.push_back(info);
    infos_.back().id = id;
    hash_to_id_.emplace(info.stable_hash, id);
    lookup_cache_.emplace(info.stable_hash, id);
    return id;
}

} // namespace ecs
} // namespace gw
