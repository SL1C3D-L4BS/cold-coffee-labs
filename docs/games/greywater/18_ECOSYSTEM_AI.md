# 18 — Greywater Ecosystem & AI

**Status:** Canonical (game-side)
**Last revised:** 2026-04-19

---

## 1. Hybrid AI architecture

Greywater combines several AI techniques, each chosen for what it does best:

| AI technique         | Use case                                 | Implementation                          |
| -------------------- | ---------------------------------------- | --------------------------------------- |
| Behavior Trees       | NPC decision-making, combat              | Custom C++ executor on ECS              |
| Utility AI           | Faction-level decisions                  | Score-based action selection            |
| GOAP                 | Complex goal planning (rare NPCs)        | Optional overlay, post-v1               |
| LLM dialogue         | NPC conversation, emergent behaviour     | llama.cpp / candle via BLD's providers  |
| RL inference         | Animation control, combat tactics        | ONNX Runtime (post-v1)                  |

Game AI is **distinct from BLD**. BLD is the developer-facing copilot; game AI is the in-world NPC and faction brain. Both may call into llama.cpp, but through different code paths — see `engine/bld/` for BLD and `engine/gameai/` for game AI.

## 2. Wildlife ecosystem

### 2.1 Predator/prey dynamics

Lightweight population simulation at the chunk level:

```cpp
struct EcosystemCell {
    f32 flora_density;      // 0.0 – 1.0
    i32 herbivore_count;
    i32 carnivore_count;

    void simulate(f32 dt) noexcept {
        flora_density += k_flora_growth * dt
                       - herbivore_count * k_herbivore_consumption * dt;
        flora_density = std::clamp(flora_density, 0.0f, 1.0f);

        f32 herb_growth = herbivore_count
                        * (flora_density * k_herbivore_birth - k_herbivore_death);
        herbivore_count += i32(herb_growth * dt);
        herbivore_count -= i32(carnivore_count * k_predation * dt);

        f32 carn_growth = carnivore_count
                        * ((herbivore_count / f32(k_max_herbivores)) * k_carnivore_birth
                           - k_carnivore_death);
        carnivore_count += i32(carn_growth * dt);
    }
};
```

Each chunk has a single `EcosystemCell`. Populations migrate between adjacent cells with a smoothing pass.

### 2.2 Spawning

Fauna spawns are probabilistic based on density maps:

```cpp
void FaunaSpawnSystem::update(World& world) {
    for (auto& cell : ecosystem_grid_.cells()) {
        if (cell.should_spawn_herbivore()) {
            spawn_herbivore(cell.position, cell.biome);
            ++cell.herbivore_count;
        }
        if (cell.should_spawn_carnivore()) {
            spawn_carnivore(cell.position, cell.biome);
            ++cell.carnivore_count;
        }
    }
}
```

### 2.3 Individual behaviour (BT)

Each spawned creature runs a Behavior Tree:

```cpp
BehaviorTree herbivore_tree = Sequence({
    Condition(is_threatened,
        Selector({
            Sequence({ Condition(can_flee),   Action(flee)   }),
            Action(defend)
        })
    ),
    Selector({
        Sequence({ Condition(is_hungry), Action(graze) }),
        Sequence({ Condition(is_tired),  Action(rest)  }),
        Action(wander)
    })
});
```

BT authoring uses vscript IR (see `docs/architecture/grey-water-engine-blueprint.md §3.8`) — designers edit BTs as text IR, view them as ImNodes graphs, and BLD can author/edit them via `vscript.*` tools.

## 3. Faction AI

### 3.1 Faction types

| Faction   | Behaviour              | Territory         | Default diplomacy   |
| --------- | ---------------------- | ----------------- | ------------------- |
| Pirates   | Aggressive, raid       | Asteroid belts    | Hostile to all      |
| Traders   | Neutral, commerce      | Space stations    | Friendly to all     |
| Military  | Defensive, patrol      | Controlled systems | Hostile to pirates |
| Cultists  | Mysterious, ritualistic | Anomaly sites    | Variable            |

### 3.2 Utility-AI decision making

Each faction evaluates available actions with a **score per action**, weighted by **considerations**:

```cpp
struct FactionAction {
    f32 base_score;
    SmallVec<Consideration, 8> considerations;
};

f32 evaluate_action(const FactionAction& action, const WorldState& state) {
    f32 score = action.base_score;
    for (const auto& c : action.considerations) {
        score *= c.evaluate(state);
    }
    return score;
}
```

The faction ticks slowly (once per minute of in-game time) and picks the highest-scoring action. Actions then spawn BT-driven NPC activities.

## 4. Space patrols and engagement

Patrol routes:

```cpp
struct PatrolRoute {
    u64                   route_id;
    SmallVec<Vec3_f64, 16> waypoints;    // orbital positions
    f32                   speed;
    u64                   faction_id;
};
```

Engagement logic checks for threats in the patrol's detection radius and transitions the patrolling entities into a combat BT.

## 5. Navigation

### 5.1 Ground: NavMesh

Greywater integrates **Recast/Detour** for ground navigation. Navmeshes are generated **per chunk** on the job system when the chunk is first needed, cached, and attached to adjacent chunks as the world streams.

### 5.2 Space: orbital pathfinding

```cpp
struct OrbitalPathfinder {
    std::vector<OrbitalWaypoint> find_path(
        Vec3_f64 start, Vec3_f64 end,
        Span<const OrbitalBody> bodies
    );
};
```

A* in 3D with gravity-assist considerations. Avoids collision with planets and stars. Minimizes delta-v. Suitable for AI ship navigation and player trajectory preview.

## 6. LLM dialogue (post-v1)

### 6.1 Async pipeline

NPC dialogue lines are generated asynchronously through llama.cpp via the job system. **No LLM call blocks the main thread.**

```cpp
class LLMDialogueSystem {
public:
    JobHandle request_dialogue(const DialogueContext& ctx) {
        return jobs_->submit([this, ctx]() {
            std::string prompt = build_prompt(ctx);
            std::string response = llama_->generate(prompt);
            callback_queue_.push({ ctx.npc_id, std::move(response) });
        });
    }

    void update() {
        while (auto r = callback_queue_.pop()) {
            deliver_dialogue(r->npc_id, r->text);
        }
    }
};
```

### 6.2 Tool use for NPC actions

NPCs can use LLM tool calls for in-world actions:

- `give_item(player, item_id, quantity)`
- `set_faction_reputation(faction, delta)`
- `spawn_entity(type, position)`
- `log_lore_event(event_id)`

This mirrors BLD's `#[bld_tool]` pattern but is entirely separate code — game AI tools live in `engine/gameai/tools/` (if this subsystem ships) and are not exposed to the developer-facing MCP server.

**Ship status:** LLM dialogue is **post-v1 Tier B** or better. The engine provides the hooks; the game ships without it in RC and adds it in a patch.

## 7. RL inference (post-v1)

```cpp
class RLInferenceSystem {
public:
    void init(const char* model_path);
    SmallVec<f32, 64> infer(Span<const f32> observation);
};
```

Use case: animation control (blend RL-predicted poses with BT-driven gameplay logic). ONNX Runtime with DirectML/CPU backends; CUDA only on hardware that has it. Definitely post-v1.

---

*The world is alive because the world remembers. Factions plan. Wildlife hunts. NPCs converse. The simulation has depth because the depth is simulated, not scripted.*
