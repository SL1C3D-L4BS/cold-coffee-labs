# ADR 0043 — Behavior Tree executor as a vscript IR evaluator

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13D)
- **Deciders:** Gameplay Lead · Simulation Lead · BLD Lead
- **Related:** ADR-0008, ADR-0009, ADR-0014, ADR-0039

## 1. Context

`docs/games/greywater/18_ECOSYSTEM_AI.md` §2.3 mandates that BT authoring goes through two paths: (a) the editor graph (ImNodes, Phase 17), and (b) BLD chat via `vscript.*` tools (ADR-0014). Both must produce **bit-identical** runtime behavior. The cheapest way to guarantee that is to make the BT executor **the same VM** that runs ADR-0008/0009 vscript IR, with BT-shaped dispatch.

The alternative — a dedicated BT bytecode — is rejected by CLAUDE.md #14 ("Visual scripting: text IR is ground-truth") and would force BLD to emit two distinct serializations.

## 2. Decision

### 2.1 Node vocabulary

BT nodes map to vscript IR opcodes:

| BT kind | vscript IR form | Status model |
|---|---|---|
| Sequence | `(seq …children)` | short-circuits on first Failure |
| Selector | `(sel …children)` | short-circuits on first Success |
| Parallel (policy All) | `(par_all N …children)` | Success when N succeed |
| Parallel (policy Any) | `(par_any N …children)` | Success when N succeed |
| Inverter | `(inv child)` | flips Success ↔ Failure |
| Succeeder | `(succ child)` | always Success |
| Repeater | `(rep N child)` | repeat up to N times |
| Cooldown | `(cooldown secs child)` | Failure while on cooldown |
| Wait | `(wait secs)` | Running until elapsed |
| Action | `(action name …args)` | dispatches to registered action fn |
| Condition | `(cond name …args)` | dispatches to registered condition fn |

`.gwvs` files gain a `(tree …)` top-level form in addition to the Phase-8 `(script …)` form. One `.gwvs` can carry both a prelude script and multiple trees.

### 2.2 Blackboard

`BlackboardComponent` owns a `SmallMap<StringId, BlackboardValue>`:

```cpp
using BlackboardValue = std::variant<
    std::int32_t, float, double, bool,
    glm::vec3, glm::dvec3,
    EntityId, StringId
>;
```

- Typed keys. Writes are type-checked. Reads fall back to a typed default if the key is missing.
- Serialization order is **declaration order** (first write wins key slot), stable across runs.
- Floats hash through the pose-hash quantization (`1e-6` precision) to avoid FMA noise poisoning the BT state hash.

### 2.3 Node factory (engine/game split)

Engine ships a small set of **generic** actions/conditions; game code registers its own via:

```cpp
void register_action   (StringId name, BTActionFn  fn, void* ctx = nullptr);
void register_condition(StringId name, BTCondFn    fn, void* ctx = nullptr);
```

Engine-generic nodes (shipped in `engine/gameai`):

| Name | Kind | Purpose |
|---|---|---|
| `log` | Action | writes a line via `gw::core::Logger` |
| `emit_event` | Action | publishes an opaque event onto the cross-subsystem bus |
| `set_bb` | Action | writes a blackboard key |
| `get_bb` | Condition | reads a blackboard key and matches a value |
| `wait` | Action | built-in timed wait |
| `has_path` | Condition | PathRequest completed with Success |
| `at_target` | Condition | distance(current, target) ≤ tolerance |
| `move_to` | Action | issues a PathRequest + drives `LocomotionComponent.velocity` |

Game-side Greywater nodes (`patrol`, `flee`, `graze`, `engage`) live in `game/greywater/ecosystem/bt_nodes.cpp` (Phase 23), registered at game boot.

### 2.4 Tick budget

- `ai.bt.fixed_dt` (default 0.1 s) governs BT tick rate. BT sub-samples off the shared anim clock.
- `ai.bt.max_steps_per_tick` (default 256) caps node visits per entity per tick — guarantees bounded worst-case work under a bugged `(rep N)` with huge N.
- Batching: 16 entities per job submission into the `sim` lane.

### 2.5 Canonical BT state hash

```
[entity_id u64]
[tree_content_hash u64]              // hash of canonical-serialized IR bytes
[active_node_path_id u32]            // deterministic id for the currently-Running node
[blackboard_serialized_bytes]        // keys sorted, floats quantized
```

All four folded into FNV-1a-64, sorted by entity id. `behavior_tree::state_hash` is the accessor; it shares the FNV primitive from `engine/physics/determinism_hash.cpp`.

### 2.6 BLD-chat authoring parity

`bt_bld_parity` CI test: two BTs (one hand-written, one BLD-chat-produced via the `vscript.create_tree` tool) that express the same patrol policy produce **bit-identical** `state_hash` after 300 BT ticks on a canned blackboard. Canonicalization (attribute sort, deterministic whitespace) is enforced at save time.

## 3. Consequences

- No second VM; no second serializer; no second debugger.
- BT hot-reload rides on Phase-8 vscript hot-reload plumbing.
- Anti-pattern avoided: Unreal-style "BT Services" that tick every frame and mutate blackboard freely — our BT only ticks at `ai.bt.fixed_dt` and all mutations go through explicit nodes.

## 4. Alternatives considered

- **Dedicated BT bytecode** — rejected (see §1).
- **Utility AI instead of BT** — deferred to Phase 23 (faction AI); BT is the framework that ecosystem AI extends.
- **GOAP** — out of scope; cost-benefit does not clear the engineering bar for Phase 13.
