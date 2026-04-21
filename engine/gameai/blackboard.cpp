// engine/gameai/blackboard.cpp — Phase 13 Wave 13E (ADR-0043 §3).

#include "engine/gameai/blackboard.hpp"

#include <string>

namespace gw::gameai {

void Blackboard::set(std::string_view key, const BBValue& v) {
    map_[std::string{key}] = v;
    ++write_count_;
}

bool Blackboard::has(std::string_view key) const noexcept {
    return map_.find(std::string{key}) != map_.end();
}

BBValue Blackboard::get(std::string_view key) const noexcept {
    auto it = map_.find(std::string{key});
    if (it == map_.end()) return {};
    return it->second;
}

void Blackboard::set_vec3(std::string_view k, const glm::vec3& v) {
    BBValue x{};
    x.kind = BBKind::Vec3;
    x.v3   = v;
    set(k, x);
}
void Blackboard::set_dvec3(std::string_view k, const glm::dvec3& v) {
    BBValue x{};
    x.kind = BBKind::DVec3;
    x.dv3  = v;
    set(k, x);
}
void Blackboard::set_entity(std::string_view k, EntityId e) {
    BBValue x{};
    x.kind   = BBKind::Entity;
    x.entity = e;
    set(k, x);
}
void Blackboard::set_string_id(std::string_view k, std::uint32_t id) {
    BBValue x{};
    x.kind      = BBKind::StringId;
    x.string_id = id;
    set(k, x);
}

} // namespace gw::gameai
