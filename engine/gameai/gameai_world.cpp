// engine/gameai/gameai_world.cpp — Phase 13 (ADR-0043, ADR-0044).
//
// Null-backend implementation of the combined BT + Navmesh facade.
// Null BT executor: recursive-descent against the topologically-sorted
// node array (root_index is evaluated last in a post-order walk, then the
// root ticks children via direct indexed access).
// Null navmesh: grid-based Dijkstra over the walkability grid with an
// 8-neighbour stencil. Deterministic by construction — no thread fan-out.

#include "engine/gameai/gameai_world.hpp"

#include "engine/core/events/event_bus.hpp"
#include "engine/jobs/scheduler.hpp"
#include "engine/physics/determinism_hash.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gw::gameai {

namespace {

// Post-order iterative tick for the null BT. Since nodes are topologically
// ordered (parent after children), we can walk each node descending into
// its children via their indices.
struct NullBTRunner {
    const BTDesc&              desc;
    Blackboard&                bb;
    const ActionRegistry&      reg;
    BTContext&                 ctx;

    // Per-node run-time scratch: cooldown timers, repeat counters, current
    // running child index, etc. Indexed by node index.
    struct Scratch {
        float         cooldown_left{0.0f};
        std::uint16_t repeat_done{0};
        std::uint16_t running_child{0};
    };
    std::vector<Scratch>& scratch;

    // Event buses (pass-through).
    events::EventBus<events::BehaviorTreeNodeEntered>*   bus_entered{nullptr};
    events::EventBus<events::BehaviorTreeNodeCompleted>* bus_completed{nullptr};

    BTStatus tick_node(std::uint16_t idx, float dt) {
        const auto& n = desc.nodes[idx];
        if (bus_entered) {
            events::BehaviorTreeNodeEntered ev{};
            ev.entity = ctx.entity;
            ev.instance = ctx.instance;
            ev.node_index = idx;
            ev.node_name = n.name;
            bus_entered->publish(ev);
        }

        BTStatus result = BTStatus::Failure;
        switch (n.kind) {
            case BTNodeKind::Sequence: {
                result = BTStatus::Success;
                for (auto c : n.children) {
                    BTStatus s = tick_node(c, dt);
                    if (s == BTStatus::Failure) { result = BTStatus::Failure; break; }
                    if (s == BTStatus::Running) { result = BTStatus::Running; break; }
                }
            } break;
            case BTNodeKind::Selector: {
                result = BTStatus::Failure;
                for (auto c : n.children) {
                    BTStatus s = tick_node(c, dt);
                    if (s == BTStatus::Success) { result = BTStatus::Success; break; }
                    if (s == BTStatus::Running) { result = BTStatus::Running; break; }
                }
            } break;
            case BTNodeKind::Parallel: {
                // Succeed-on-all semantics (ADR-0043 §2.3). Stable order.
                std::uint32_t succ = 0, fail = 0;
                for (auto c : n.children) {
                    BTStatus s = tick_node(c, dt);
                    if (s == BTStatus::Success) ++succ;
                    else if (s == BTStatus::Failure) ++fail;
                }
                if (succ == n.children.size()) result = BTStatus::Success;
                else if (fail > 0)             result = BTStatus::Failure;
                else                           result = BTStatus::Running;
            } break;
            case BTNodeKind::Inverter: {
                const BTStatus s = tick_node(n.children[0], dt);
                if (s == BTStatus::Success) result = BTStatus::Failure;
                else if (s == BTStatus::Failure) result = BTStatus::Success;
                else result = BTStatus::Running;
            } break;
            case BTNodeKind::Succeeder: {
                (void)tick_node(n.children[0], dt);
                result = BTStatus::Success;
            } break;
            case BTNodeKind::RepeatN: {
                auto& sc = scratch[idx];
                BTStatus inner = BTStatus::Success;
                while (sc.repeat_done < n.repeat_count) {
                    inner = tick_node(n.children[0], dt);
                    if (inner == BTStatus::Running) { result = BTStatus::Running; break; }
                    ++sc.repeat_done;
                }
                if (sc.repeat_done >= n.repeat_count) {
                    sc.repeat_done = 0;
                    result = inner;
                }
            } break;
            case BTNodeKind::UntilSuccess: {
                BTStatus inner = tick_node(n.children[0], dt);
                if (inner == BTStatus::Success) result = BTStatus::Success;
                else result = BTStatus::Running;
            } break;
            case BTNodeKind::UntilFailure: {
                BTStatus inner = tick_node(n.children[0], dt);
                if (inner == BTStatus::Failure) result = BTStatus::Success;
                else result = BTStatus::Running;
            } break;
            case BTNodeKind::Cooldown: {
                auto& sc = scratch[idx];
                if (sc.cooldown_left > 0.0f) {
                    sc.cooldown_left = std::max(0.0f, sc.cooldown_left - dt);
                    result = BTStatus::Failure;
                } else {
                    const BTStatus inner = tick_node(n.children[0], dt);
                    if (inner != BTStatus::Running) sc.cooldown_left = n.cooldown_s;
                    result = inner;
                }
            } break;
            case BTNodeKind::ActionLeaf:    result = reg.invoke_action   (n.name, ctx); break;
            case BTNodeKind::ConditionLeaf: result = reg.invoke_condition(n.name, ctx) ? BTStatus::Success : BTStatus::Failure; break;
            case BTNodeKind::BBSetFloat:    { BBValue v{}; v.kind = BBKind::Float;  v.f32 = n.f_value; bb.set(n.bb_key, v); result = BTStatus::Success; } break;
            case BTNodeKind::BBSetInt:      { BBValue v{}; v.kind = BBKind::Int32;  v.i32 = n.i_value; bb.set(n.bb_key, v); result = BTStatus::Success; } break;
            case BTNodeKind::BBSetBool:     { BBValue v{}; v.kind = BBKind::Bool;   v.b   = n.b_value; bb.set(n.bb_key, v); result = BTStatus::Success; } break;
            case BTNodeKind::BBCompareGT: {
                const BBValue v = bb.get(n.bb_key);
                const float cmp = (v.kind == BBKind::Int32) ? static_cast<float>(v.i32) : v.f32;
                result = (cmp > n.f_value) ? BTStatus::Success : BTStatus::Failure;
            } break;
            case BTNodeKind::BBCompareLT: {
                const BBValue v = bb.get(n.bb_key);
                const float cmp = (v.kind == BBKind::Int32) ? static_cast<float>(v.i32) : v.f32;
                result = (cmp < n.f_value) ? BTStatus::Success : BTStatus::Failure;
            } break;
            case BTNodeKind::BBCompareEQ: {
                const BBValue v = bb.get(n.bb_key);
                const float cmp = (v.kind == BBKind::Int32) ? static_cast<float>(v.i32) : v.f32;
                result = (std::fabs(cmp - n.f_value) < 1e-5f) ? BTStatus::Success : BTStatus::Failure;
            } break;
        }

        if (bus_completed) {
            events::BehaviorTreeNodeCompleted ev{};
            ev.entity = ctx.entity;
            ev.instance = ctx.instance;
            ev.node_index = idx;
            ev.status = result;
            bus_completed->publish(ev);
        }
        return result;
    }
};

// ---- Navmesh grid helpers ----

struct GridInfo {
    std::int32_t tiles_x{0};
    std::int32_t tiles_z{0};
    std::int32_t cells_per_tile{1};
    std::int32_t cells_x{0};
    std::int32_t cells_z{0};
    float        cell_size_m{0.3f};
    glm::vec3    origin_ws{0.0f};
};

GridInfo make_grid(const NavmeshDesc& d) {
    GridInfo g{};
    g.tiles_x = d.tile_count_x;
    g.tiles_z = d.tile_count_z;
    g.cell_size_m = d.cell_size_m;
    g.cells_per_tile = std::max(1, static_cast<std::int32_t>(d.tile_size_m / d.cell_size_m));
    g.cells_x = g.tiles_x * g.cells_per_tile;
    g.cells_z = g.tiles_z * g.cells_per_tile;
    g.origin_ws = d.origin_ws;
    return g;
}

inline std::size_t cell_index(const GridInfo& g, std::int32_t cx, std::int32_t cz) noexcept {
    return static_cast<std::size_t>(cz) * static_cast<std::size_t>(g.cells_x)
         + static_cast<std::size_t>(cx);
}

inline bool ws_to_cell(const GridInfo& g, const glm::vec3& ws, std::int32_t& cx, std::int32_t& cz) noexcept {
    const float fx = (ws.x - g.origin_ws.x) / g.cell_size_m;
    const float fz = (ws.z - g.origin_ws.z) / g.cell_size_m;
    cx = static_cast<std::int32_t>(std::floor(fx));
    cz = static_cast<std::int32_t>(std::floor(fz));
    if (cx < 0 || cz < 0 || cx >= g.cells_x || cz >= g.cells_z) return false;
    return true;
}

inline glm::vec3 cell_to_ws(const GridInfo& g, std::int32_t cx, std::int32_t cz) noexcept {
    return glm::vec3{
        g.origin_ws.x + (static_cast<float>(cx) + 0.5f) * g.cell_size_m,
        g.origin_ws.y,
        g.origin_ws.z + (static_cast<float>(cz) + 0.5f) * g.cell_size_m,
    };
}

} // namespace

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------
struct NullTree {
    BTDesc desc{};
    bool   alive{false};
    std::uint64_t content_hash{0};
};

struct NullBTInstance {
    BehaviorTreeHandle tree{};
    EntityId           entity{kEntityNone};
    Blackboard         blackboard{};
    std::vector<NullBTRunner::Scratch> scratch{};
    BTStatus           last_status{BTStatus::Invalid};
    bool               alive{false};
    std::uint32_t      generation{0};
};

struct NullNavmesh {
    NavmeshDesc     desc{};
    GridInfo        grid{};
    std::uint64_t   content_hash{0};
    bool            alive{false};
};

struct NullAgent {
    NavmeshHandle     navmesh{};
    EntityId          entity{kEntityNone};
    glm::vec3         position_ws{0.0f};
    glm::vec3         velocity{0.0f};
    float             max_speed{1.5f};
    std::vector<glm::vec3> path{};
    std::size_t       next_waypoint{0};
    bool              arrived{true};
    bool              alive{false};
};

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct GameAIWorld::Impl {
    GameAIConfig                     cfg{};
    events::CrossSubsystemBus*       cross_bus{nullptr};
    jobs::Scheduler*                 scheduler{nullptr};

    std::vector<NullTree>            trees{1};
    std::vector<NullBTInstance>      bt_instances{1};
    std::vector<NullNavmesh>         navmeshes{1};
    std::vector<NullAgent>           agents{1};

    std::vector<std::uint32_t>       free_trees{};
    std::vector<std::uint32_t>       free_instances{};
    std::vector<std::uint32_t>       free_navmeshes{};
    std::vector<std::uint32_t>       free_agents{};

    ActionRegistry                   registry{};

    events::EventBus<events::BehaviorTreeNodeEntered>   bus_node_entered{};
    events::EventBus<events::BehaviorTreeNodeCompleted> bus_node_completed{};
    events::EventBus<events::BlackboardValueChanged>    bus_bb_changed{};
    events::EventBus<events::NavmeshTileBaked>          bus_tile_baked{};
    events::EventBus<events::NavmeshTileUnloaded>       bus_tile_unloaded{};
    events::EventBus<events::PathQueryStarted>          bus_path_started{};
    events::EventBus<events::PathQueryCompleted>        bus_path_completed{};
    events::EventBus<events::NavAgentArrived>           bus_agent_arrived{};

    bool           initialized_flag{false};
    bool           paused_flag{false};
    float          bt_accumulator_s{0.0f};
    std::uint64_t  frame_index{0};
    std::uint32_t  debug_flag_mask{0};

    // ---- accessors ----
    NullTree*    tree_ptr(BehaviorTreeHandle h) noexcept {
        if (!h.valid() || h.id >= trees.size()) return nullptr;
        auto& t = trees[h.id];
        return t.alive ? &t : nullptr;
    }
    const NullTree* tree_ptr(BehaviorTreeHandle h) const noexcept {
        if (!h.valid() || h.id >= trees.size()) return nullptr;
        const auto& t = trees[h.id];
        return t.alive ? &t : nullptr;
    }
    NullBTInstance* inst_ptr(BehaviorInstanceHandle h) noexcept {
        if (!h.valid() || h.id >= bt_instances.size()) return nullptr;
        auto& i = bt_instances[h.id];
        return i.alive ? &i : nullptr;
    }
    const NullBTInstance* inst_ptr(BehaviorInstanceHandle h) const noexcept {
        if (!h.valid() || h.id >= bt_instances.size()) return nullptr;
        const auto& i = bt_instances[h.id];
        return i.alive ? &i : nullptr;
    }
    NullNavmesh* nav_ptr(NavmeshHandle h) noexcept {
        if (!h.valid() || h.id >= navmeshes.size()) return nullptr;
        auto& n = navmeshes[h.id];
        return n.alive ? &n : nullptr;
    }
    const NullNavmesh* nav_ptr(NavmeshHandle h) const noexcept {
        if (!h.valid() || h.id >= navmeshes.size()) return nullptr;
        const auto& n = navmeshes[h.id];
        return n.alive ? &n : nullptr;
    }
    NullAgent* agent_ptr(AgentHandle h) noexcept {
        if (!h.valid() || h.id >= agents.size()) return nullptr;
        auto& a = agents[h.id];
        return a.alive ? &a : nullptr;
    }
    const NullAgent* agent_ptr(AgentHandle h) const noexcept {
        if (!h.valid() || h.id >= agents.size()) return nullptr;
        const auto& a = agents[h.id];
        return a.alive ? &a : nullptr;
    }

    // ---- Dijkstra over the walkability grid ----
    PathStatus run_dijkstra(const NullNavmesh& nav, const glm::vec3& start_ws, const glm::vec3& end_ws,
                            float max_length_m, std::vector<glm::vec3>& out_waypoints,
                            float& out_length_m) const {
        out_waypoints.clear();
        out_length_m = 0.0f;
        std::int32_t sx, sz, ex, ez;
        if (!ws_to_cell(nav.grid, start_ws, sx, sz)) return PathStatus::StartNotOnMesh;
        if (!ws_to_cell(nav.grid, end_ws,   ex, ez)) return PathStatus::EndNotOnMesh;
        const std::size_t total = static_cast<std::size_t>(nav.grid.cells_x) * static_cast<std::size_t>(nav.grid.cells_z);
        if (nav.desc.walkable.size() < total) return PathStatus::NavmeshNotLoaded;
        if (!nav.desc.walkable[cell_index(nav.grid, sx, sz)]) return PathStatus::StartNotOnMesh;
        if (!nav.desc.walkable[cell_index(nav.grid, ex, ez)]) return PathStatus::EndNotOnMesh;

        // Priority-queue Dijkstra; cost is cell_size_m * step-count (1 or √2).
        std::vector<float>         dist(total, std::numeric_limits<float>::infinity());
        std::vector<std::uint32_t> prev(total, std::numeric_limits<std::uint32_t>::max());
        using Node = std::pair<float, std::uint32_t>;  // {dist, idx}
        auto cmp = [](const Node& a, const Node& b) { return a.first > b.first; };
        std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
        const std::uint32_t s_idx = static_cast<std::uint32_t>(cell_index(nav.grid, sx, sz));
        dist[s_idx] = 0.0f;
        pq.push({0.0f, s_idx});

        constexpr int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        constexpr int dz8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

        const float cell = nav.grid.cell_size_m;
        const std::uint32_t e_idx = static_cast<std::uint32_t>(cell_index(nav.grid, ex, ez));
        bool found = false;
        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();
            if (d > dist[u]) continue;
            if (u == e_idx) { found = true; break; }
            if (d > max_length_m) continue;
            const std::int32_t ux = static_cast<std::int32_t>(u % static_cast<std::uint32_t>(nav.grid.cells_x));
            const std::int32_t uz = static_cast<std::int32_t>(u / static_cast<std::uint32_t>(nav.grid.cells_x));
            for (int k = 0; k < 8; ++k) {
                const std::int32_t nx = ux + dx8[k];
                const std::int32_t nz = uz + dz8[k];
                if (nx < 0 || nz < 0 || nx >= nav.grid.cells_x || nz >= nav.grid.cells_z) continue;
                const std::size_t n_idx = cell_index(nav.grid, nx, nz);
                if (!nav.desc.walkable[n_idx]) continue;
                const float step = (dx8[k] != 0 && dz8[k] != 0) ? 1.41421356f : 1.0f;
                const float nd = d + step * cell;
                if (nd < dist[n_idx]) {
                    dist[n_idx] = nd;
                    prev[n_idx] = u;
                    pq.push({nd, static_cast<std::uint32_t>(n_idx)});
                }
            }
        }
        if (!found) return PathStatus::NoPath;

        // Reconstruct.
        std::vector<std::uint32_t> reversed;
        std::uint32_t cur = e_idx;
        while (cur != std::numeric_limits<std::uint32_t>::max()) {
            reversed.push_back(cur);
            if (cur == s_idx) break;
            cur = prev[cur];
        }
        out_waypoints.reserve(reversed.size());
        for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
            const std::int32_t cx = static_cast<std::int32_t>(*it % static_cast<std::uint32_t>(nav.grid.cells_x));
            const std::int32_t cz = static_cast<std::int32_t>(*it / static_cast<std::uint32_t>(nav.grid.cells_x));
            out_waypoints.push_back(cell_to_ws(nav.grid, cx, cz));
        }
        // Replace first/last with exact start/end (center-cell projection was used
        // only for routing).
        if (!out_waypoints.empty()) {
            out_waypoints.front() = start_ws;
            out_waypoints.back()  = end_ws;
        }
        out_length_m = dist[e_idx];
        return PathStatus::Ok;
    }

    std::uint64_t hash_path_output(const PathQueryOutput& o) const {
        std::uint64_t h = gw::physics::kFnvOffset64;
        auto fold = [&](const void* p, std::size_t n) {
            h = gw::physics::fnv1a64_combine(h,
                std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
        };
        const std::uint8_t sbyte = static_cast<std::uint8_t>(o.status);
        fold(&sbyte, sizeof(sbyte));
        const std::uint64_t wc = o.waypoints.size();
        fold(&wc, sizeof(wc));
        // Fold each waypoint quantized to micrometers.
        for (const auto& w : o.waypoints) {
            const std::int64_t qx = static_cast<std::int64_t>(std::llround(static_cast<double>(w.x) * 1e6));
            const std::int64_t qy = static_cast<std::int64_t>(std::llround(static_cast<double>(w.y) * 1e6));
            const std::int64_t qz = static_cast<std::int64_t>(std::llround(static_cast<double>(w.z) * 1e6));
            fold(&qx, sizeof(qx));
            fold(&qy, sizeof(qy));
            fold(&qz, sizeof(qz));
        }
        const std::int64_t qlen = static_cast<std::int64_t>(std::llround(static_cast<double>(o.length_m) * 1e6));
        fold(&qlen, sizeof(qlen));
        return h;
    }
};

// ---------------------------------------------------------------------------
// GameAIWorld public API — thin dispatchers.
// ---------------------------------------------------------------------------
GameAIWorld::GameAIWorld() : impl_(std::make_unique<Impl>()) {}
GameAIWorld::~GameAIWorld() = default;

bool GameAIWorld::initialize(const GameAIConfig& cfg,
                             events::CrossSubsystemBus* bus,
                             jobs::Scheduler* scheduler) {
    if (impl_->initialized_flag) return true;
    impl_->cfg = cfg;
    impl_->cross_bus = bus;
    impl_->scheduler = scheduler;
    impl_->initialized_flag = true;
    impl_->frame_index = 0;
    impl_->debug_flag_mask = 0;
    if (cfg.bt_tick_hz > 0.0f) impl_->cfg.bt_fixed_dt = 1.0f / cfg.bt_tick_hz;
    return true;
}

void GameAIWorld::shutdown() {
    if (!impl_->initialized_flag) return;
    impl_->trees.assign(1, NullTree{});
    impl_->bt_instances.assign(1, NullBTInstance{});
    impl_->navmeshes.assign(1, NullNavmesh{});
    impl_->agents.assign(1, NullAgent{});
    impl_->free_trees.clear();
    impl_->free_instances.clear();
    impl_->free_navmeshes.clear();
    impl_->free_agents.clear();
    impl_->bt_accumulator_s = 0.0f;
    impl_->frame_index = 0;
    impl_->paused_flag = false;
    impl_->initialized_flag = false;
}

bool GameAIWorld::initialized() const noexcept { return impl_->initialized_flag; }
BackendKind GameAIWorld::backend() const noexcept { return BackendKind::Null; }
std::string_view GameAIWorld::backend_name() const noexcept { return "null"; }
const GameAIConfig& GameAIWorld::config() const noexcept { return impl_->cfg; }

// ---- Behavior trees ----
BehaviorTreeHandle GameAIWorld::create_tree(const BTDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_trees.empty()) {
        id = impl_->free_trees.back();
        impl_->free_trees.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->trees.size());
        impl_->trees.push_back(NullTree{});
    }
    auto& t = impl_->trees[id];
    t = NullTree{};
    t.desc = desc;
    t.content_hash = bt_content_hash(desc);
    t.alive = true;
    return BehaviorTreeHandle{id};
}

void GameAIWorld::destroy_tree(BehaviorTreeHandle h) noexcept {
    auto* t = impl_->tree_ptr(h);
    if (!t) return;
    t->alive = false;
    impl_->free_trees.push_back(h.id);
}

std::uint64_t GameAIWorld::tree_hash(BehaviorTreeHandle h) const noexcept {
    const auto* t = impl_->tree_ptr(h);
    return t ? t->content_hash : 0ULL;
}

BehaviorInstanceHandle GameAIWorld::create_bt_instance(BehaviorTreeHandle tree, EntityId entity) {
    if (!impl_->tree_ptr(tree)) return {};
    std::uint32_t id;
    if (!impl_->free_instances.empty()) {
        id = impl_->free_instances.back();
        impl_->free_instances.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->bt_instances.size());
        impl_->bt_instances.push_back(NullBTInstance{});
    }
    auto& i = impl_->bt_instances[id];
    i = NullBTInstance{};
    i.alive = true;
    i.tree = tree;
    i.entity = entity;
    ++i.generation;
    const auto* t = impl_->tree_ptr(tree);
    i.scratch.assign(t->desc.nodes.size(), NullBTRunner::Scratch{});
    return BehaviorInstanceHandle{id};
}

void GameAIWorld::destroy_bt_instance(BehaviorInstanceHandle h) noexcept {
    auto* i = impl_->inst_ptr(h);
    if (!i) return;
    i->alive = false;
    impl_->free_instances.push_back(h.id);
}

std::size_t GameAIWorld::bt_instance_count() const noexcept {
    std::size_t n = 0;
    for (const auto& i : impl_->bt_instances) if (i.alive) ++n;
    return n;
}

Blackboard* GameAIWorld::blackboard(BehaviorInstanceHandle h) noexcept {
    auto* i = impl_->inst_ptr(h);
    return i ? &i->blackboard : nullptr;
}
const Blackboard* GameAIWorld::blackboard(BehaviorInstanceHandle h) const noexcept {
    const auto* i = impl_->inst_ptr(h);
    return i ? &i->blackboard : nullptr;
}

ActionRegistry&       GameAIWorld::action_registry()       noexcept { return impl_->registry; }
const ActionRegistry& GameAIWorld::action_registry() const noexcept { return impl_->registry; }

BTStatus GameAIWorld::last_status(BehaviorInstanceHandle h) const noexcept {
    const auto* i = impl_->inst_ptr(h);
    return i ? i->last_status : BTStatus::Invalid;
}

std::uint32_t GameAIWorld::tick_bt(float delta_time_s) {
    if (!impl_->initialized_flag || impl_->paused_flag) return 0;
    impl_->bt_accumulator_s += delta_time_s;
    const float dt = impl_->cfg.bt_fixed_dt;
    std::uint32_t n = 0;
    // Cap at one tick per step() call when already above the sub-rate
    // threshold; callers that want more substeps invoke step() in a loop.
    while (impl_->bt_accumulator_s >= dt && n < 4u) {
        tick_bt_fixed();
        impl_->bt_accumulator_s -= dt;
        ++n;
    }
    return n;
}

void GameAIWorld::tick_bt_fixed() {
    const float dt = impl_->cfg.bt_fixed_dt;
    for (std::uint32_t iid = 1; iid < impl_->bt_instances.size(); ++iid) {
        auto& inst = impl_->bt_instances[iid];
        if (!inst.alive) continue;
        const auto* t = impl_->tree_ptr(inst.tree);
        if (!t) continue;

        BTContext ctx{};
        ctx.instance = BehaviorInstanceHandle{iid};
        ctx.entity   = inst.entity;
        ctx.blackboard = &inst.blackboard;
        ctx.dt_s     = dt;

        NullBTRunner runner{t->desc, inst.blackboard, impl_->registry, ctx, inst.scratch,
                            &impl_->bus_node_entered, &impl_->bus_node_completed};
        inst.last_status = runner.tick_node(t->desc.root_index, dt);
    }
}

// ---- Navmesh ----
NavmeshHandle GameAIWorld::create_navmesh(const NavmeshDesc& desc) {
    std::uint32_t id;
    if (!impl_->free_navmeshes.empty()) {
        id = impl_->free_navmeshes.back();
        impl_->free_navmeshes.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->navmeshes.size());
        impl_->navmeshes.push_back(NullNavmesh{});
    }
    auto& n = impl_->navmeshes[id];
    n = NullNavmesh{};
    n.desc = desc;
    n.grid = make_grid(desc);
    n.content_hash = navmesh_content_hash(desc);
    n.alive = true;
    // Fire a Baked event per tile.
    for (std::int32_t tz = 0; tz < desc.tile_count_z; ++tz) {
        for (std::int32_t tx = 0; tx < desc.tile_count_x; ++tx) {
            events::NavmeshTileBaked ev{NavmeshHandle{id}, tx, tz, n.content_hash};
            impl_->bus_tile_baked.publish(ev);
        }
    }
    return NavmeshHandle{id};
}

void GameAIWorld::destroy_navmesh(NavmeshHandle h) noexcept {
    auto* n = impl_->nav_ptr(h);
    if (!n) return;
    n->alive = false;
    impl_->free_navmeshes.push_back(h.id);
}

NavmeshInfo GameAIWorld::navmesh_info(NavmeshHandle h) const noexcept {
    const auto* n = impl_->nav_ptr(h);
    if (!n) return {};
    NavmeshInfo info{};
    info.tile_count_x = n->desc.tile_count_x;
    info.tile_count_z = n->desc.tile_count_z;
    info.content_hash = n->content_hash;
    info.walkable_cells = 0;
    for (auto b : n->desc.walkable) if (b) ++info.walkable_cells;
    return info;
}

std::uint64_t GameAIWorld::navmesh_hash(NavmeshHandle h) const noexcept {
    const auto* n = impl_->nav_ptr(h);
    return n ? n->content_hash : 0ULL;
}

void GameAIWorld::rebake_tile(NavmeshHandle h, std::int32_t tx, std::int32_t tz) {
    auto* n = impl_->nav_ptr(h);
    if (!n) return;
    if (tx < 0 || tz < 0 || tx >= n->desc.tile_count_x || tz >= n->desc.tile_count_z) return;
    // Re-compute overall content hash (Phase-13 null backend: tile granularity
    // collapses to a whole-mesh rehash).
    n->content_hash = navmesh_content_hash(n->desc);
    events::NavmeshTileBaked ev{h, tx, tz, n->content_hash};
    impl_->bus_tile_baked.publish(ev);
}

void GameAIWorld::unload_tile(NavmeshHandle h, std::int32_t tx, std::int32_t tz) {
    auto* n = impl_->nav_ptr(h);
    if (!n) return;
    if (tx < 0 || tz < 0 || tx >= n->desc.tile_count_x || tz >= n->desc.tile_count_z) return;
    // Null backend: mark every cell in this tile as non-walkable.
    const auto& g = n->grid;
    const std::int32_t cx0 = tx * g.cells_per_tile;
    const std::int32_t cz0 = tz * g.cells_per_tile;
    for (std::int32_t dz = 0; dz < g.cells_per_tile; ++dz) {
        for (std::int32_t dx = 0; dx < g.cells_per_tile; ++dx) {
            const std::size_t idx = cell_index(g, cx0 + dx, cz0 + dz);
            if (idx < n->desc.walkable.size()) n->desc.walkable[idx] = 0;
        }
    }
    n->content_hash = navmesh_content_hash(n->desc);
    impl_->bus_tile_unloaded.publish(events::NavmeshTileUnloaded{h, tx, tz});
}

bool GameAIWorld::find_path(const PathQueryInput& in, PathQueryOutput& out) const {
    out = PathQueryOutput{};
    const auto* n = impl_->nav_ptr(in.navmesh);
    if (!n) { out.status = PathStatus::NavmeshNotLoaded; return false; }
    impl_->bus_path_started.publish(events::PathQueryStarted{in.entity, in.start_ws, in.end_ws});
    out.status = impl_->run_dijkstra(*n, in.start_ws, in.end_ws, in.max_length_m,
                                     out.waypoints, out.length_m);
    out.result_hash = impl_->hash_path_output(out);
    impl_->bus_path_completed.publish(events::PathQueryCompleted{
        in.entity, out.status, out.length_m,
        static_cast<std::uint16_t>(out.waypoints.size()), out.result_hash});
    return out.status == PathStatus::Ok;
}

void GameAIWorld::find_path_batch(std::span<const PathQueryInput> in,
                                  std::span<PathQueryOutput>      out) const {
    const std::size_t m = std::min(in.size(), out.size());
    for (std::size_t i = 0; i < m; ++i) (void)find_path(in[i], out[i]);
}

bool GameAIWorld::nearest_walkable(NavmeshHandle nav,
                                   const glm::vec3& from_ws,
                                   glm::vec3& out_ws) const noexcept {
    const auto* n = impl_->nav_ptr(nav);
    if (!n) return false;
    std::int32_t cx, cz;
    if (!ws_to_cell(n->grid, from_ws, cx, cz)) return false;
    const float hysteresis = n->desc.cell_size_m * 2.0f;
    const std::int32_t radius_cells = static_cast<std::int32_t>(
        std::max(1.0f, std::ceil(hysteresis / n->desc.cell_size_m)));
    for (std::int32_t r = 0; r <= radius_cells; ++r) {
        for (std::int32_t dz = -r; dz <= r; ++dz) {
            for (std::int32_t dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) != r && std::abs(dz) != r) continue;
                const std::int32_t nx = cx + dx, nz = cz + dz;
                if (nx < 0 || nz < 0 || nx >= n->grid.cells_x || nz >= n->grid.cells_z) continue;
                const std::size_t idx = cell_index(n->grid, nx, nz);
                if (idx < n->desc.walkable.size() && n->desc.walkable[idx]) {
                    out_ws = cell_to_ws(n->grid, nx, nz);
                    return true;
                }
            }
        }
    }
    return false;
}

// ---- Agents ----
AgentHandle GameAIWorld::create_agent(NavmeshHandle nav, EntityId entity, const glm::vec3& pos_ws) {
    std::uint32_t id;
    if (!impl_->free_agents.empty()) {
        id = impl_->free_agents.back();
        impl_->free_agents.pop_back();
    } else {
        id = static_cast<std::uint32_t>(impl_->agents.size());
        impl_->agents.push_back(NullAgent{});
    }
    auto& a = impl_->agents[id];
    a = NullAgent{};
    a.alive = true;
    a.navmesh = nav;
    a.entity = entity;
    a.position_ws = pos_ws;
    a.arrived = true;
    return AgentHandle{id};
}

void GameAIWorld::destroy_agent(AgentHandle h) noexcept {
    auto* a = impl_->agent_ptr(h);
    if (!a) return;
    a->alive = false;
    impl_->free_agents.push_back(h.id);
}

void GameAIWorld::set_agent_target(AgentHandle h, const glm::vec3& target_ws) {
    auto* a = impl_->agent_ptr(h);
    if (!a) return;
    PathQueryInput in{};
    in.navmesh = a->navmesh;
    in.entity  = a->entity;
    in.start_ws = a->position_ws;
    in.end_ws   = target_ws;
    PathQueryOutput out{};
    (void)find_path(in, out);
    a->path = std::move(out.waypoints);
    a->next_waypoint = a->path.empty() ? 0 : 1;
    a->arrived = a->path.empty();
}

glm::vec3 GameAIWorld::agent_position(AgentHandle h) const noexcept {
    const auto* a = impl_->agent_ptr(h);
    return a ? a->position_ws : glm::vec3{0.0f};
}

bool GameAIWorld::agent_arrived(AgentHandle h) const noexcept {
    const auto* a = impl_->agent_ptr(h);
    return a ? a->arrived : false;
}

std::uint32_t GameAIWorld::tick_agents(float delta_time_s) {
    std::uint32_t count = 0;
    for (std::uint32_t id = 1; id < impl_->agents.size(); ++id) {
        auto& a = impl_->agents[id];
        if (!a.alive) continue;
        ++count;
        if (a.arrived || a.path.empty()) continue;
        if (a.next_waypoint >= a.path.size()) { a.arrived = true; continue; }
        const glm::vec3 tgt = a.path[a.next_waypoint];
        const glm::vec3 delta = tgt - a.position_ws;
        const float d = glm::length(delta);
        const float step = a.max_speed * delta_time_s;
        if (d <= step) {
            a.position_ws = tgt;
            ++a.next_waypoint;
            if (a.next_waypoint >= a.path.size()) {
                a.arrived = true;
                impl_->bus_agent_arrived.publish(events::NavAgentArrived{
                    a.entity, AgentHandle{id}, a.position_ws});
            }
        } else {
            a.position_ws += (delta / d) * step;
        }
    }
    return count;
}

// ---- Simulation wrappers ----
std::uint32_t GameAIWorld::step(float delta_time_s) {
    if (!impl_->initialized_flag || impl_->paused_flag) return 0;
    const std::uint32_t n = tick_bt(delta_time_s);
    (void)tick_agents(delta_time_s);
    ++impl_->frame_index;
    return n;
}

void GameAIWorld::step_fixed() { tick_bt_fixed(); ++impl_->frame_index; }

float GameAIWorld::interpolation_alpha() const noexcept {
    if (!impl_->initialized_flag) return 0.0f;
    const float dt = impl_->cfg.bt_fixed_dt;
    return std::clamp(impl_->bt_accumulator_s / dt, 0.0f, 1.0f);
}

void GameAIWorld::reset() {
    if (!impl_->initialized_flag) return;
    impl_->trees.assign(1, NullTree{});
    impl_->bt_instances.assign(1, NullBTInstance{});
    impl_->navmeshes.assign(1, NullNavmesh{});
    impl_->agents.assign(1, NullAgent{});
    impl_->free_trees.clear();
    impl_->free_instances.clear();
    impl_->free_navmeshes.clear();
    impl_->free_agents.clear();
    impl_->bt_accumulator_s = 0.0f;
    impl_->frame_index = 0;
}

void GameAIWorld::pause(bool paused) noexcept { impl_->paused_flag = paused; }
bool GameAIWorld::paused() const noexcept     { return impl_->paused_flag; }
std::uint64_t GameAIWorld::frame_index() const noexcept { return impl_->frame_index; }

// ---- Determinism ----
std::uint64_t GameAIWorld::determinism_hash() const {
    const std::uint64_t a = bt_state_hash();
    const std::uint64_t b = nav_content_hash();
    std::uint64_t h = gw::physics::kFnvOffset64;
    std::uint8_t bytes[16];
    std::memcpy(bytes,     &a, 8);
    std::memcpy(bytes + 8, &b, 8);
    h = gw::physics::fnv1a64_combine(h, {bytes, 16});
    return h;
}

std::uint64_t GameAIWorld::bt_state_hash() const {
    std::uint64_t h = gw::physics::kFnvOffset64;
    auto fold = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };
    std::uint64_t alive = 0;
    for (const auto& i : impl_->bt_instances) if (i.alive) ++alive;
    fold(&alive, sizeof(alive));
    for (std::uint32_t id = 1; id < impl_->bt_instances.size(); ++id) {
        const auto& i = impl_->bt_instances[id];
        if (!i.alive) continue;
        fold(&id, sizeof(id));
        const std::uint8_t st = static_cast<std::uint8_t>(i.last_status);
        fold(&st, sizeof(st));
        // Blackboard: iterate entries in sorted-key order for determinism.
        std::vector<const std::pair<const std::string, BBValue>*> ents;
        ents.reserve(i.blackboard.entries().size());
        for (const auto& kv : i.blackboard.entries()) ents.push_back(&kv);
        std::sort(ents.begin(), ents.end(), [](auto a, auto b){ return a->first < b->first; });
        const std::uint64_t ec = ents.size();
        fold(&ec, sizeof(ec));
        for (const auto* kv : ents) {
            const std::uint64_t kl = kv->first.size();
            fold(&kl, sizeof(kl));
            fold(kv->first.data(), kv->first.size());
            const std::uint8_t kk = static_cast<std::uint8_t>(kv->second.kind);
            fold(&kk, sizeof(kk));
            // Fold the active variant.
            switch (kv->second.kind) {
                case BBKind::Int32:    fold(&kv->second.i32,    sizeof(std::int32_t)); break;
                case BBKind::Float:    fold(&kv->second.f32,    sizeof(float));         break;
                case BBKind::Double:   fold(&kv->second.f64,    sizeof(double));        break;
                case BBKind::Bool:     { std::uint8_t b = kv->second.b ? 1u : 0u; fold(&b, 1); } break;
                case BBKind::Vec3:     fold(&kv->second.v3,     sizeof(glm::vec3));     break;
                case BBKind::DVec3:    fold(&kv->second.dv3,    sizeof(glm::dvec3));    break;
                case BBKind::Entity:   fold(&kv->second.entity, sizeof(EntityId));      break;
                case BBKind::StringId: fold(&kv->second.string_id, sizeof(std::uint32_t)); break;
            }
        }
    }
    return h;
}

std::uint64_t GameAIWorld::nav_content_hash() const {
    std::uint64_t h = gw::physics::kFnvOffset64;
    auto fold = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };
    std::uint64_t alive = 0;
    for (const auto& nm : impl_->navmeshes) if (nm.alive) ++alive;
    fold(&alive, sizeof(alive));
    for (std::uint32_t id = 1; id < impl_->navmeshes.size(); ++id) {
        const auto& nm = impl_->navmeshes[id];
        if (!nm.alive) continue;
        fold(&id, sizeof(id));
        fold(&nm.content_hash, sizeof(nm.content_hash));
    }
    return h;
}

void GameAIWorld::set_debug_flag_mask(std::uint32_t mask) noexcept { impl_->debug_flag_mask = mask; }
std::uint32_t GameAIWorld::debug_flag_mask() const noexcept        { return impl_->debug_flag_mask; }

events::EventBus<events::BehaviorTreeNodeEntered>&   GameAIWorld::bus_node_entered()   noexcept { return impl_->bus_node_entered; }
events::EventBus<events::BehaviorTreeNodeCompleted>& GameAIWorld::bus_node_completed() noexcept { return impl_->bus_node_completed; }
events::EventBus<events::BlackboardValueChanged>&    GameAIWorld::bus_bb_changed()     noexcept { return impl_->bus_bb_changed; }
events::EventBus<events::NavmeshTileBaked>&          GameAIWorld::bus_tile_baked()     noexcept { return impl_->bus_tile_baked; }
events::EventBus<events::NavmeshTileUnloaded>&       GameAIWorld::bus_tile_unloaded()  noexcept { return impl_->bus_tile_unloaded; }
events::EventBus<events::PathQueryStarted>&          GameAIWorld::bus_path_started()   noexcept { return impl_->bus_path_started; }
events::EventBus<events::PathQueryCompleted>&        GameAIWorld::bus_path_completed() noexcept { return impl_->bus_path_completed; }
events::EventBus<events::NavAgentArrived>&           GameAIWorld::bus_agent_arrived()  noexcept { return impl_->bus_agent_arrived; }

// ---- Free functions ----
std::uint64_t navmesh_content_hash(const NavmeshDesc& desc) noexcept {
    std::uint64_t h = gw::physics::kFnvOffset64;
    auto fold = [&](const void* p, std::size_t n) {
        h = gw::physics::fnv1a64_combine(h,
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(p), n});
    };
    const std::uint64_t nl = desc.name.size();
    fold(&nl, sizeof(nl));
    fold(desc.name.data(), desc.name.size());
    fold(&desc.tile_count_x, sizeof(desc.tile_count_x));
    fold(&desc.tile_count_z, sizeof(desc.tile_count_z));
    fold(&desc.tile_size_m,  sizeof(desc.tile_size_m));
    fold(&desc.cell_size_m,  sizeof(desc.cell_size_m));
    fold(&desc.agent_radius_m,      sizeof(desc.agent_radius_m));
    fold(&desc.agent_height_m,      sizeof(desc.agent_height_m));
    fold(&desc.agent_max_climb_m,   sizeof(desc.agent_max_climb_m));
    fold(&desc.agent_max_slope_deg, sizeof(desc.agent_max_slope_deg));
    fold(&desc.origin_ws,           sizeof(desc.origin_ws));
    const std::uint64_t wc = desc.walkable.size();
    fold(&wc, sizeof(wc));
    if (!desc.walkable.empty()) fold(desc.walkable.data(), desc.walkable.size());
    return h;
}

} // namespace gw::gameai
