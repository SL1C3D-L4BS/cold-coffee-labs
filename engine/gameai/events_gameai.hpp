#pragma once
// engine/gameai/events_gameai.hpp — Phase 13 Wave 13E (ADR-0043 / 0044).
//
// Game-AI events travel through the Phase-11 EventBus.

#include "engine/gameai/gameai_types.hpp"
#include "engine/gameai/navmesh.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

namespace gw::events {

struct BehaviorTreeNodeEntered {
    gw::gameai::EntityId               entity{gw::gameai::kEntityNone};
    gw::gameai::BehaviorInstanceHandle instance{};
    std::uint16_t                      node_index{0};
    std::string                        node_name{};
};

struct BehaviorTreeNodeCompleted {
    gw::gameai::EntityId               entity{gw::gameai::kEntityNone};
    gw::gameai::BehaviorInstanceHandle instance{};
    std::uint16_t                      node_index{0};
    gw::gameai::BTStatus               status{gw::gameai::BTStatus::Invalid};
};

struct BlackboardValueChanged {
    gw::gameai::EntityId               entity{gw::gameai::kEntityNone};
    gw::gameai::BehaviorInstanceHandle instance{};
    std::string                        key{};
    gw::gameai::BBKind                 kind{gw::gameai::BBKind::Bool};
};

struct NavmeshTileBaked {
    gw::gameai::NavmeshHandle          navmesh{};
    std::int32_t                       tile_x{gw::gameai::kInvalidTile};
    std::int32_t                       tile_z{gw::gameai::kInvalidTile};
    std::uint64_t                      tile_content_hash{0};
};

struct NavmeshTileUnloaded {
    gw::gameai::NavmeshHandle          navmesh{};
    std::int32_t                       tile_x{gw::gameai::kInvalidTile};
    std::int32_t                       tile_z{gw::gameai::kInvalidTile};
};

struct PathQueryStarted {
    gw::gameai::EntityId               entity{gw::gameai::kEntityNone};
    glm::vec3                          start_ws{0.0f};
    glm::vec3                          end_ws{0.0f};
};

struct PathQueryCompleted {
    gw::gameai::EntityId               entity{gw::gameai::kEntityNone};
    gw::gameai::PathStatus             status{gw::gameai::PathStatus::NoPath};
    float                              length_m{0.0f};
    std::uint16_t                      waypoint_count{0};
    std::uint64_t                      result_hash{0};
};

struct NavAgentArrived {
    gw::gameai::EntityId               entity{gw::gameai::kEntityNone};
    gw::gameai::AgentHandle            agent{};
    glm::vec3                          position_ws{0.0f};
};

} // namespace gw::events
