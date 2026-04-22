// engine/ecs/component_registry.cpp
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.4.

#include "component_registry.hpp"

#include <stdexcept>

namespace gw::ecs {

ComponentRegistry::ComponentRegistry() {
    infos_.reserve(64);
    hash_to_id_.reserve(64);
    lookup_cache_.reserve(64);
}

ComponentRegistry::~ComponentRegistry() = default;

ComponentRegistry::ComponentRegistry(ComponentRegistry&&) noexcept            = default;
ComponentRegistry& ComponentRegistry::operator=(ComponentRegistry&&) noexcept = default;

const ComponentTypeInfo& ComponentRegistry::info(ComponentTypeId component_type_id) const {
    if (component_type_id >= infos_.size()) {
        throw std::out_of_range("ComponentRegistry::info: invalid ComponentTypeId");
    }
    return infos_[component_type_id];
}

std::optional<ComponentTypeId>
ComponentRegistry::lookup_by_hash(std::uint64_t stable_hash) const noexcept {
    const auto found = hash_to_id_.find(stable_hash);
    if (found == hash_to_id_.end()) {
        return std::nullopt;
    }
    return found->second;
}

ComponentTypeId ComponentRegistry::register_raw(const ComponentTypeInfo& info) {
    if (const auto found = hash_to_id_.find(info.stable_hash);
        found != hash_to_id_.end()) {
        return found->second;
    }
    if (infos_.size() >= kMaxComponentTypes) {
        throw std::runtime_error(
            "ComponentRegistry: kMaxComponentTypes exceeded; raise the limit "
            "in entity.hpp or stop registering new component types");
    }
    const auto new_component_type_id = static_cast<ComponentTypeId>(infos_.size());
    infos_.push_back(info);
    infos_.back().id = new_component_type_id;
    hash_to_id_.emplace(info.stable_hash, new_component_type_id);
    lookup_cache_.emplace(info.stable_hash, new_component_type_id);
    return new_component_type_id;
}

} // namespace gw::ecs
