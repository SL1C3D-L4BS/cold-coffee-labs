# ADR 0004 — ECS World (archetype-of-chunks, ComponentRegistry, queries, hierarchy)

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `docs/AUDIT_MAP_2026-04-20.md §P1-11` (Phase-3 drift); `docs/05 §Phase-3 amendment note`; `CLAUDE.md` non-negotiables #5 (no raw `new`/`delete` in engine), #9 (no STL in hot paths — we will qualify this), #20 (doctrine first); `docs/02_SYSTEMS_AND_SIMULATIONS.md §ECS`; `docs/03_VULKAN_RESEARCH_GUIDE.md §Scene representation`.

## 1. Context

Phase 3 weeks 014–015 nominally delivered the ECS: generational entity handles, archetype chunk storage, typed queries, system registration. The audit on 2026-04-20 (P1-11) found this is false. Today the tree has:

- `engine/ecs/chunk.{hpp,cpp}` — one `Chunk` per archetype, fixed slot count, byte-array per `ComponentType` slot.
- `engine/ecs/archetype.hpp` — 32-bit bitmask identity.
- `engine/ecs/component.hpp` — abstract `ComponentStorage<T>` with `for_each` declared-not-defined.
- `engine/ecs/system.{hpp,cpp}` — `SystemRegistry::register_system<T>` with an inverted `static_assert(std::is_base_of_v<T, System>, …)` (won't compile if instantiated).
- `engine/ecs/entity.hpp` — `EntityHandle = uint32_t`; contradicts `editor/selection/selection_context.hpp` which uses `uint64_t`.
- No `World` type, no `ComponentRegistry`, no queries, no multi-chunk storage, no entity migration, no tests.

*Editor v0.1* (Phase 7) is the first subsystem that needs a real ECS: the outliner walks the scene tree, the inspector binds to components, PIE enter/exit needs to snapshot the world (requires serialization → requires a world to serialize). The correction is a ground-up ECS-World implementation folded into Phase 7 under Path-A (see `docs/05 §Phase-3 amendment`).

This ADR fixes the design decisions so implementation doesn't re-litigate them mid-flight.

## 2. Decision

### 2.1 Top-level shape: `gw::ecs::World` owns everything

```cpp
namespace gw::ecs {

class World {
public:
    World();
    ~World();

    World(const World&)            = delete;
    World& operator=(const World&) = delete;
    World(World&&) noexcept;
    World& operator=(World&&) noexcept;

    // Entity lifecycle
    [[nodiscard]] Entity create_entity();
    void                 destroy_entity(Entity);
    [[nodiscard]] bool   is_alive(Entity) const noexcept;

    // Component add/remove (triggers archetype migration)
    template <typename T>
    T& add_component(Entity, T value);
    template <typename T>
    void remove_component(Entity);
    template <typename T>
    [[nodiscard]] bool has_component(Entity) const noexcept;
    template <typename T>
    [[nodiscard]] T* get_component(Entity);       // nullptr if missing
    template <typename T>
    [[nodiscard]] const T* get_component(Entity) const;

    // Queries — see §2.5
    template <typename... Cs, typename Fn>
    void for_each(Fn&& fn);

    // Bulk access for serialization / editor inspection
    [[nodiscard]] std::size_t entity_count() const noexcept;
    void visit_entities(std::function<void(Entity)>) const;   // stable order
    [[nodiscard]] const ComponentRegistry& component_registry() const noexcept;

    // Access internal archetype table (reflection / inspector / serializer only)
    [[nodiscard]] const ArchetypeTable& archetype_table() const noexcept;

private:
    ComponentRegistry registry_;
    ArchetypeTable    archetypes_;       // all archetypes, owned here
    EntityTable       entity_table_;     // generational slot table
    // more implementation-private state — see §2.3
};

} // namespace gw::ecs
```

`World` is the only public entry point. Systems, queries, and serialization all go through it. No global state.

### 2.2 `Entity` (handle) — 64-bit, replacing the 32-bit `EntityHandle`

```cpp
struct Entity {
    std::uint64_t bits = 0;   // 0 == null

    [[nodiscard]] std::uint32_t index()      const noexcept { return static_cast<std::uint32_t>(bits); }
    [[nodiscard]] std::uint32_t generation() const noexcept { return static_cast<std::uint32_t>(bits >> 32); }

    [[nodiscard]] bool is_null() const noexcept { return bits == 0; }
    [[nodiscard]] bool operator==(Entity) const noexcept = default;

    static constexpr Entity null() noexcept { return {0}; }
};
static_assert(sizeof(Entity) == 8);
```

- **Layout:** `{generation:32, index:32}` packed into `bits`. Generation 0 is reserved to mean "never used / destroyed", so a freshly-created entity starts at generation 1. This makes `Entity{0}` the unambiguous null handle.
- **Editor integration:** `editor/selection/selection_context.hpp` currently uses its own `uint64_t EntityHandle`. It will be retypedef'd as `using EntityHandle = gw::ecs::Entity;` in the same PR that lands World. The 32-bit `gw::ecs::EntityHandle` in `engine/ecs/entity.hpp` is replaced; callers that relied on the old enum-indexed `ComponentType` switch to the component-registry path (§2.4).
- **Why 64-bit?** 32-bit generation + 32-bit index. The index is a table row number in `EntityTable`; the generation is bumped on destroy. 32 bits of index supports 4 billion entities — well past any realistic world size. 32 bits of generation makes handle-reuse bugs effectively impossible on human timescales. The editor's pre-existing choice of `uint64_t` is retroactively correct.

### 2.3 Storage: **archetype-of-chunks**

- An **Archetype** is uniquely identified by its sorted `std::vector<ComponentTypeId>` (the canonical component set). An `ArchetypeId` is a small index (registered monotonically in an `ArchetypeTable`).
- Each Archetype owns **N Chunks**. A Chunk is a fixed-size block (default **16 KiB**) partitioned into parallel per-component arrays (SOA). Slot count per chunk is determined by the archetype's total per-entity component footprint: `slots_per_chunk = floor(chunk_bytes / sum(component_size))`, clamped to `[1, UINT16_MAX]`.
- A chunk stores entities densely: indices `0..live_count-1` are live; `live_count..slots_per_chunk-1` are free. No per-slot free-list within a chunk — on destroy, the last live entity is swapped into the freed slot (O(1), preserves density). This requires updating the swapped entity's `EntityTable` row.
- A new chunk is allocated when all current chunks of an archetype are full. Chunks are never deallocated while the archetype exists — memory grows; compaction is a Phase-8+ concern.
- Per-entity metadata (archetype-id, chunk-index, slot-index) lives in the **EntityTable**, indexed by `Entity.index()`. Layout:

```cpp
struct EntitySlot {
    // Monotonically-increasing version counter. Bumped on *both* create and
    // destroy so a handle captured pre-destroy never satisfies is_alive again,
    // even after its slot is reused.
    std::uint32_t generation   = 0;
    std::uint32_t archetype_id = std::numeric_limits<std::uint32_t>::max();
    std::uint16_t chunk_index  = std::numeric_limits<std::uint16_t>::max();
    std::uint16_t slot_index   = std::numeric_limits<std::uint16_t>::max();
    // True iff the slot currently represents a live entity. Distinct from
    // `generation` so destroy can advance generation (defeating stale handles)
    // without reusing 0 as a "free" marker — 0 stays reserved for Entity::null().
    bool          alive        = false;
};
```

That's 16 bytes per live-or-freed entity slot (12 meaningful + 1 flag + 3 padding on common ABIs). `EntityTable` is a `std::vector<EntitySlot>` (grows; never shrinks). **Freed indices are re-used via a `std::vector<std::uint32_t> free_list_`** — grab from the back of the list in O(1). If the free list is empty, push a new slot.

**Generation-bump rule (2026-04-21 amendment).** Both `create_entity` and `destroy_entity` bump `generation` (skipping 0 on wrap). Without destroy-side bumping, a destroy→create sequence on the same slot left the fresh handle's generation equal to the destroyed one, silently re-validating stale copies. The `alive` flag is the authoritative liveness bit; generation is the version stamp used for stale-handle rejection. See `tests/unit/ecs/world_test.cpp "destroy_entity invalidates the handle; reuse bumps generation"`.

### 2.4 `ComponentRegistry`

Registers every component type at program start (usually before `World` construction, but re-registering an already-registered type is idempotent). Each registration records:

```cpp
struct ComponentTypeInfo {
    ComponentTypeId    id;                    // monotonic small int (u16)
    std::size_t        size        = 0;
    std::size_t        alignment   = 0;
    bool               trivially_copyable = false;
    std::uint64_t      stable_hash = 0;       // fnv1a of fully-qualified type name

    // Non-trivial lifecycle hooks (only populated when !trivially_copyable).
    void (*copy_construct)(void* dst, const void* src) = nullptr;
    void (*move_construct)(void* dst, void* src) noexcept = nullptr;
    void (*destruct)(void* p) noexcept = nullptr;

    // Optional: pointer to the reflection TypeInfo if the type has GW_REFLECT.
    // Populated via ADL lookup of `describe(T*)`. Null for non-reflected types.
    const gw::core::TypeInfo* reflection = nullptr;

    // Stable debug name (never freed; points into static storage).
    std::string_view debug_name;
};

class ComponentRegistry {
public:
    template <typename T>
    ComponentTypeId register_component();            // idempotent; returns canonical id

    template <typename T>
    [[nodiscard]] ComponentTypeId id_of() const;     // throws if not registered

    [[nodiscard]] const ComponentTypeInfo& info(ComponentTypeId) const;
    [[nodiscard]] std::size_t component_count() const noexcept;

    // Stable-hash → id lookup (used by serialization).
    [[nodiscard]] std::optional<ComponentTypeId> lookup_by_hash(std::uint64_t) const;
};
```

- **`id_of<T>()`** uses a per-type static `ComponentTypeId` storage cached on first lookup, zero-cost thereafter.
- `stable_hash` is FNV-1a of the fully-qualified type name obtained from a compile-time `gw_type_name<T>()` helper (same trick `editor/reflect/reflect.hpp` uses). It is the **serialization key**, not `id` — `id` is not portable across builds.
- `trivially_copyable` is `std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>`. When true, entity migration / serialization can use `std::memcpy`; when false, the function-pointer hooks are used.
- `ComponentRegistry` owns the `ComponentTypeInfo` storage (one `std::vector` + one `std::unordered_map` for name-hash lookup).
- **Registration location:** each component type provides `GW_ECS_COMPONENT(T)` at its definition site (a macro that expands to `inline gw_ecs_register_component<T>()`-style static registration via a TU-local constructor). The specifics live in the implementation — the ADR just freezes the surface.

### 2.5 Queries

```cpp
// Compile-time assembled query: iterate every archetype that contains
// every listed component, invoking `fn(Entity, Cs&...)` for each live slot.
template <typename... Cs, typename Fn>
void World::for_each(Fn&& fn);
```

- Implementation: find `archetype_ids` whose component set is a **superset** of `{id_of<Cs>()...}`. Walk matching archetypes → walk their chunks → walk their live slots → gather the N component pointers from per-chunk SOA arrays and call `fn`.
- Queries do **not** cache; each call re-walks the archetype table. Phase-8+ can add a `QueryCache` keyed on the component-set mask if profiling shows this is hot. Today the world sizes we care about (editor scenes, PIE sessions) have O(10³) entities max — walking archetypes is negligible.
- **No `With<A, B> / Without<C>`-style filter DSL in this ADR.** `for_each<A, B>` means "entities with at least A and B." If we need exclusion later we add `for_each_without<...>`.
- **No parallel queries in this ADR.** Single-threaded only. Parallel-query dispatch via `engine/jobs/` is a Phase-11+ item; it needs archetype-level locks and we don't have the use case yet.
- **Query stability under add/remove.** Iterating while adding/removing components is **undefined**. Callers defer structural changes with a `World::defer_*` API (Phase-8 addition) or collect-then-apply. Today the editor doesn't mutate during iteration, so we don't need the deferred API yet — but we **document** the invariant.

### 2.6 Entity migration (add/remove component)

Adding or removing a component on a live entity moves it from its current archetype `A` to the destination archetype `B = A ∪ {T}` or `B = A \ {T}`:

1. Look up or lazily create archetype `B` in `ArchetypeTable`.
2. Allocate a slot in a chunk of `B`.
3. For each component type in `A ∩ B`: copy (trivial → `memcpy`, non-trivial → `move_construct`) from the source chunk to the destination chunk. Destruct the source (non-trivial only).
4. For the added component (add path): construct in the destination using the caller-supplied value.
5. Compact the source chunk: swap-back the last-live-entity into the freed slot; update its `EntityTable` row.
6. Update this entity's `EntityTable` row with the new `{archetype_id, chunk_index, slot_index}`. Generation unchanged.

### 2.7 Hierarchy (scene tree)

Hierarchy is represented as a **dedicated component**, not a field on `TransformComponent`. This is the Unity/Unreal-ish choice: composition > baked-in-transform.

```cpp
struct HierarchyComponent {
    Entity parent       = Entity::null();
    Entity first_child  = Entity::null();
    Entity next_sibling = Entity::null();
    // Leaf if first_child.is_null(); root if parent.is_null().
};
```

**Why a separate component?**

1. Entities can participate in the hierarchy without having a Transform (folder entities, metadata entities, scene-root markers).
2. The archetype query `for_each<HierarchyComponent>` cleanly iterates only hierarchy-participating entities; `for_each<TransformComponent>` iterates only spatial ones.
3. Adding/removing an entity from the hierarchy = add/remove the component. No need for "hierarchy disabled" bit inside Transform.

**Traversal API** lives on `World` as non-templated helpers:

```cpp
[[nodiscard]] std::vector<Entity> children_of(Entity) const;        // convenience
void reparent(Entity child, Entity new_parent);                     // handles linked-list fixup
void visit_descendants(Entity root, std::function<void(Entity)>) const;   // DFS
void visit_roots(std::function<void(Entity)>) const;                // entities whose HierarchyComponent.parent.is_null()
```

Cycle detection (`reparent(A, descendant-of-A)` rejected) is the responsibility of `reparent`. It returns a `bool`/`Result`; the editor's reparent command handles the failure path.

### 2.8 Systems

**Deferred to Phase 8+.** The broken `SystemRegistry` is deleted in the same PR as World lands; gameplay code (the `greywater_gameplay.dll` module + engine-internal subsystems that care about ECS — e.g. the outliner rebuild path) iterate the World directly via `for_each`. Adding a `System` abstraction that manages scheduling and read/write declarations is a separate design problem with its own ADR when we actually have multiple interacting systems.

The Phase-3 week-015 card ("typed queries, system registration") is split: **queries ship here; system registration ships in a future ADR.**

### 2.9 Memory policy — STL usage

`CLAUDE.md` NN #9 forbids STL containers in engine *hot paths*. The ECS:

- Uses `std::vector` / `std::unordered_map` / `std::span` / `std::function` liberally in management surfaces (entity table, archetype table, registry, query dispatch).
- Does **not** use STL containers per-entity or per-component. Chunks are byte-array allocations; SOA arrays are raw pointers into that buffer.
- Does **not** allocate per-`for_each`-iteration. Query dispatch walks cached archetype-id vectors and passes raw component pointers to the callback.

This is consistent with how `engine/jobs/` uses `std::vector` for worker-count-sized lists but not per-job: NN #9 is about *hot-path per-frame per-element allocations*, not about using `std::vector` for fixed-size management metadata. The hot path here is "walk N slots, call `fn`" — zero allocations, no STL traversal.

### 2.10 Thread-safety

Single-threaded. `World` is not safe for concurrent access. This is a documented invariant, not a latent bug: PIE runs single-threaded today, the editor runs single-threaded today, and the asset loader / cooker touch assets, not the ECS world.

If we ever need parallel queries, they get a dedicated ADR.

## 3. Consequences

### Immediate (this commit block)

- `engine/ecs/world.{hpp,cpp}`, `engine/ecs/component_registry.{hpp,cpp}`, `engine/ecs/entity.hpp` (rewritten), `engine/ecs/archetype.{hpp,cpp}` (rewritten — no more bitmask, replaced with sorted-type-id vector + `ArchetypeTable`) land in `engine/ecs/`.
- `engine/ecs/chunk.{hpp,cpp}` are deleted or folded into `archetype.cpp` — the single-archetype `Chunk` primitive has no role once `Archetype` owns multi-chunk storage directly.
- `engine/ecs/system.{hpp,cpp}` are **deleted** (broken static_assert, no consumers). System registration is re-addressed in a later ADR.
- `engine/ecs/component.{hpp,cpp}` are **deleted** (abstract `ComponentStorage<T>` with undefined `for_each` has no consumers and no working implementation).
- `editor/selection/selection_context.hpp` retypedefs `EntityHandle = gw::ecs::Entity`.
- Hook-site fallout: audit for every mention of `gw::ecs::EntityHandle` / `gw::ecs::ComponentType` / `gw::ecs::SystemRegistry` / `gw::ecs::Chunk` / `gw::ecs::ComponentStorage` in the tree and migrate. Evidence so far: those types are almost entirely self-referential within `engine/ecs/` and untouched by consumers — migration is local.
- ECS tests land under `tests/unit/ecs/`: entity create/destroy, add/remove component → archetype migration, query iteration, hierarchy reparent, handle staleness under re-use, 10 000-entity stress.

### Short term (Phase 7 remainder)

- Outliner walks `World::visit_roots` + `World::visit_descendants`.
- Inspector reads components via `World::get_component<T>` + `ComponentRegistry::info(id).reflection` → `WidgetDrawer::draw_field`.
- PIE snapshot/restore uses `ComponentRegistry.stable_hash` as the type key (ADR-0007 — ECS serialization).

### Long term

- Deferred structural changes (`World::defer_destroy` / `defer_add` / `defer_remove` with a flush point) come in the first PR that needs to mutate during iteration.
- Parallel queries + System scheduling (Phase 11+) get their own ADR.
- Archetype compaction (reclaim empty chunks) is a Phase-11+ memory-hygiene item.
- `QueryCache` is added only if profiling shows archetype walks dominating frame time (unlikely below ~10⁴ archetypes).

### Alternatives considered

1. **Sparse-set ECS (EnTT style).** Rejected. EnTT's per-component sparse arrays are excellent for cache-coherent iteration of a single component, but archetype iteration (multi-component queries, which we do constantly in rendering) costs extra indirection. Unity's DOTS / Bevy's approach of archetype-of-chunks is a better fit for our usage profile and gives us a natural serialization granularity.

2. **Hash-map-per-component (CEDR-like, handle → value).** Rejected. Scales poorly past ~10⁴ entities, cache-hostile, and offers no natural archetype to hang serialization and snapshot tests on.

3. **Bitmask archetype identity (what the old `archetype.hpp` did).** Rejected at 32 bits (max 32 component types — will trip before end of game). At 128/256 bits it works but is less ergonomic than sorted-type-id vector + hash — the vector gives stable iteration order for free, which serialization needs.

4. **Keep `EntityHandle = uint32_t`, widen editor to match engine.** Rejected. The editor's `uint64_t` is the right choice (more generation bits = fewer stale-handle bugs); engine's `uint32_t` was a premature optimization at a time when we had no profile data suggesting handle size mattered.

5. **Hierarchy as a field on `TransformComponent`.** Rejected per §2.7.

6. **Ship queries with a filter DSL (`With<A> & Without<B>`) now.** Rejected as premature; YAGNI until we have a call site.

## 4. References

- `docs/AUDIT_MAP_2026-04-20.md §P1-11`
- `docs/05_ROADMAP_AND_MILESTONES.md §Phase-3 amendment`
- Unity DOTS archetype memory model — [ECS Concepts: Archetype](https://docs.unity3d.com/Packages/com.unity.entities@1.3/manual/concepts-archetypes.html)
- Bevy ECS book — [Archetypes, tables, sparse sets](https://bevy-cheatbook.github.io/programming/ecs-intro.html)
- Our own `docs/02_SYSTEMS_AND_SIMULATIONS.md §ECS` — general framing
- `CLAUDE.md` non-negotiables #5, #9, #20

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20. Implementation tracked via the todo list on the same session.*
