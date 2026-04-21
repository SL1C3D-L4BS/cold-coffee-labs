#pragma once
// engine/gameai/gameai_world.hpp — Phase 13 (ADR-0043, ADR-0044).
//
// PIMPL facade over both sub-systems that Phase 13 adds to "game AI":
//   * BehaviorTreeWorld — fixed-sub-rate BT executor with Blackboards.
//   * NavmeshWorld      — Recast/Detour-compatible tile grid + path query.
//
// The two live under the same facade because they share determinism
// cadence, debug-draw mask, and hash fold (ADR-0046 §3). Either can be
// used independently: create a GameAIWorld, skip one sub-API.
//
// No Recast / Detour headers leak through this file.

#include "engine/gameai/behavior_tree.hpp"
#include "engine/gameai/blackboard.hpp"
#include "engine/gameai/events_gameai.hpp"
#include "engine/gameai/gameai_config.hpp"
#include "engine/gameai/gameai_types.hpp"
#include "engine/gameai/navmesh.hpp"

#include "engine/core/events/event_bus.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace gw::events { class CrossSubsystemBus; }
namespace gw::jobs   { class Scheduler; }

namespace gw::gameai {

enum class BackendKind : std::uint8_t {
    Null   = 0,
    Recast = 1,
};

class GameAIWorld {
public:
    GameAIWorld();
    ~GameAIWorld();

    GameAIWorld(const GameAIWorld&)            = delete;
    GameAIWorld& operator=(const GameAIWorld&) = delete;
    GameAIWorld(GameAIWorld&&)                 = delete;
    GameAIWorld& operator=(GameAIWorld&&)      = delete;

    [[nodiscard]] bool initialize(const GameAIConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] BackendKind backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;
    [[nodiscard]] const GameAIConfig& config() const noexcept;

    // ---- Behavior Trees ----------------------------------------------------
    [[nodiscard]] BehaviorTreeHandle     create_tree(const BTDesc& desc);
    void                                 destroy_tree(BehaviorTreeHandle h) noexcept;
    [[nodiscard]] std::uint64_t          tree_hash(BehaviorTreeHandle h) const noexcept;

    [[nodiscard]] BehaviorInstanceHandle create_bt_instance(BehaviorTreeHandle tree, EntityId entity);
    void                                 destroy_bt_instance(BehaviorInstanceHandle h) noexcept;
    [[nodiscard]] std::size_t            bt_instance_count() const noexcept;

    [[nodiscard]] Blackboard*            blackboard(BehaviorInstanceHandle h) noexcept;
    [[nodiscard]] const Blackboard*      blackboard(BehaviorInstanceHandle h) const noexcept;

    [[nodiscard]] ActionRegistry&        action_registry() noexcept;
    [[nodiscard]] const ActionRegistry&  action_registry() const noexcept;

    [[nodiscard]] BTStatus               last_status(BehaviorInstanceHandle h) const noexcept;

    // Tick the BT executor. Sub-rate: BT ticks at `bt_tick_hz`, so one
    // `tick(dt)` call may execute 0 or 1 BT ticks depending on the
    // accumulator. Returns the number of BT ticks executed.
    [[nodiscard]] std::uint32_t          tick_bt(float delta_time_s);
    void                                 tick_bt_fixed();

    // ---- Navmesh ------------------------------------------------------------
    [[nodiscard]] NavmeshHandle          create_navmesh(const NavmeshDesc& desc);
    void                                 destroy_navmesh(NavmeshHandle h) noexcept;
    [[nodiscard]] NavmeshInfo            navmesh_info(NavmeshHandle h) const noexcept;
    [[nodiscard]] std::uint64_t          navmesh_hash(NavmeshHandle h) const noexcept;

    // Rebake a single tile after its walkability grid has been modified.
    // `tx`/`tz` must lie inside the navmesh's tile grid.
    void                                 rebake_tile(NavmeshHandle h, std::int32_t tx, std::int32_t tz);
    void                                 unload_tile(NavmeshHandle h, std::int32_t tx, std::int32_t tz);

    // Single + batched path queries.
    [[nodiscard]] bool                   find_path(const PathQueryInput& in, PathQueryOutput& out) const;
    void                                 find_path_batch(std::span<const PathQueryInput> in,
                                                         std::span<PathQueryOutput>      out) const;

    // Nearest walkable point on the mesh. Returns false if the query is too
    // far from any walkable cell (hysteresis = `cell_size * 2`).
    [[nodiscard]] bool                   nearest_walkable(NavmeshHandle nav,
                                                          const glm::vec3& from_ws,
                                                          glm::vec3& out_ws) const noexcept;

    // ---- Agents (driven by BT leaves / gameplay code) -----------------------
    [[nodiscard]] AgentHandle            create_agent(NavmeshHandle nav,
                                                      EntityId entity,
                                                      const glm::vec3& pos_ws);
    void                                 destroy_agent(AgentHandle h) noexcept;

    void                                 set_agent_target(AgentHandle h, const glm::vec3& target_ws);
    [[nodiscard]] glm::vec3              agent_position(AgentHandle h) const noexcept;
    [[nodiscard]] bool                   agent_arrived(AgentHandle h) const noexcept;

    // Integrate agents along their current paths at the BT sub-rate.
    // Returns number of agents integrated.
    std::uint32_t                        tick_agents(float delta_time_s);

    // ---- Simulation ----------------------------------------------------------
    [[nodiscard]] std::uint32_t          step(float delta_time_s);
    void                                 step_fixed();
    [[nodiscard]] float                  interpolation_alpha() const noexcept;

    void                                 reset();
    void                                 pause(bool paused) noexcept;
    [[nodiscard]] bool                   paused() const noexcept;
    [[nodiscard]] std::uint64_t          frame_index() const noexcept;

    // ---- Determinism ---------------------------------------------------------
    [[nodiscard]] std::uint64_t          determinism_hash() const;
    [[nodiscard]] std::uint64_t          bt_state_hash() const;
    [[nodiscard]] std::uint64_t          nav_content_hash() const;

    // ---- Debug draw ---------------------------------------------------------
    enum class DebugFlag : std::uint32_t {
        NavmeshTiles  = 1u << 0,
        NavmeshPolys  = 1u << 1,
        Paths         = 1u << 2,
        Agents        = 1u << 3,
        BTActiveNodes = 1u << 4,
        Blackboard    = 1u << 5,
        All           = 0xFFFFFFFFu,
    };
    void set_debug_flag_mask(std::uint32_t mask) noexcept;
    [[nodiscard]] std::uint32_t debug_flag_mask() const noexcept;

    // ---- Event buses (owned) -----------------------------------------------
    [[nodiscard]] events::EventBus<events::BehaviorTreeNodeEntered>&   bus_node_entered()   noexcept;
    [[nodiscard]] events::EventBus<events::BehaviorTreeNodeCompleted>& bus_node_completed() noexcept;
    [[nodiscard]] events::EventBus<events::BlackboardValueChanged>&    bus_bb_changed()     noexcept;
    [[nodiscard]] events::EventBus<events::NavmeshTileBaked>&          bus_tile_baked()     noexcept;
    [[nodiscard]] events::EventBus<events::NavmeshTileUnloaded>&       bus_tile_unloaded()  noexcept;
    [[nodiscard]] events::EventBus<events::PathQueryStarted>&          bus_path_started()   noexcept;
    [[nodiscard]] events::EventBus<events::PathQueryCompleted>&        bus_path_completed() noexcept;
    [[nodiscard]] events::EventBus<events::NavAgentArrived>&           bus_agent_arrived()  noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::gameai
