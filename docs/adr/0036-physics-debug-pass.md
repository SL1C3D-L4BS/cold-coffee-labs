# ADR 0036 — physics_debug_pass — frame-graph integration

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 12 Wave 12D)
- **Deciders:** Cold Coffee Labs engine + render group
- **Relates:** ADR-0031, ADR-0027 (ui_pass), Phase-5 frame graph, Phase-7 editor gizmos

## Context

Physics debugging without a visualization is blind. Jolt ships a `DebugRenderer` abstraction; we must adapt it to Greywater's frame-graph + render-HAL without creating a second line/triangle renderer.

## Decision

### 1. Public API

```cpp
namespace gw::physics {

enum class DebugDrawFlag : std::uint32_t {
    None         = 0,
    Bodies       = 1u << 0,     // AABBs + shape outlines
    Contacts     = 1u << 1,     // contact points + normals
    BroadPhase   = 1u << 2,     // broadphase tree cells
    Constraints  = 1u << 3,     // joint frames + limits
    Characters   = 1u << 4,     // capsule + ground normal
    ActiveIslands= 1u << 5,
    Velocities   = 1u << 6,
    Triggers     = 1u << 7,
};
using DebugDrawFlags = std::uint32_t;

struct DebugLine  { glm::vec3 a; glm::vec3 b; std::uint32_t rgba; };
struct DebugTri   { glm::vec3 a; glm::vec3 b; glm::vec3 c; std::uint32_t rgba; };

class DebugDrawSink {
public:
    virtual ~DebugDrawSink() = default;
    virtual void line(const DebugLine&) = 0;
    virtual void triangle(const DebugTri&) = 0;
    virtual void text(glm::vec3 pos_ws, std::string_view s, std::uint32_t rgba) = 0;
};

class PhysicsWorld {
    void set_debug_flags(DebugDrawFlags f);
    DebugDrawFlags debug_flags() const noexcept;
    // Emit debug geometry into the sink. Called from the render pass; safe
    // to call between substeps (reads only post-step state).
    void debug_draw(DebugDrawSink&) const;
};

} // namespace gw::physics
```

### 2. Frame-graph node

`engine/render/passes/physics_debug_pass.{hpp,cpp}` defines a `FrameGraphNode` that runs **after** `ui_pass` so physics overlays sit above HUD but below developer-console output. It:

1. Reads `PhysicsWorld::debug_flags()` — zero means the pass is skipped at record time.
2. Calls `PhysicsWorld::debug_draw(sink)` where `sink` is an adapter that writes into a transient GPU line buffer shared with the editor gizmos pipeline (Phase 7).
3. Binds the same line/triangle pipeline from `engine/render/debug_draw.{hpp,cpp}`.
4. Caps lines per frame at `physics.debug.max_lines` (default 32 768). Over-cap frames drop the tail and log a single throttled warning.

**The pass produces no Vulkan commands when flags == 0.** CLAUDE.md non-negotiable #1 (RX 580 baseline): no measurable frame-time when the developer toggle is off.

### 3. Release builds

In release builds the frame-graph node compiles to an empty class unless `dev.console.allow_release` is true. Retail players never see physics debug draw by accident.

### 4. CVar mapping

Each flag has a companion bool CVar that the settings binder can toggle:

| Flag | CVar |
|---|---|
| `Bodies`       | `physics.debug.bodies` |
| `Contacts`     | `physics.debug.contacts` |
| `BroadPhase`   | `physics.debug.broadphase` |
| `Constraints`  | `physics.debug.constraints` |
| `Characters`   | `physics.debug.character` |
| `ActiveIslands`| `physics.debug.islands` |
| `Velocities`   | `physics.debug.velocities` |
| `Triggers`     | `physics.debug.triggers` |

The master `physics.debug.enabled` CVar gates the pass end-to-end. A CVar change publishes `ConfigCVarChanged`; a small subscriber in `runtime::Engine` recomputes the flag mask and pushes it via `PhysicsWorld::set_debug_flags`.

### 5. Console

Console `physics.debug` command accepts tokens (whitespace-separated) and toggles the flags in one shot:

```
physics.debug bodies contacts character
physics.debug off
physics.debug all
physics.debug help
```

## Consequences

- One line/triangle pipeline for editor gizmos + physics debug + future VFX debug.
- Debug draw does not break the determinism contract — `debug_draw()` is `const`.
- The pass is free when off; perf gate `physics_debug_pass_off_overhead ≤ 0.05 ms`.

## Alternatives considered

- **ImGui draw lists.** Rejected — in-game surface; ImGui is editor-only (CLAUDE.md non-negotiable #12).
- **Separate pipeline per flag.** Rejected — redundant; one pipeline handles all line + triangle draws.
- **Draw commands recorded by physics on the job threads.** Rejected — the render thread owns Vulkan command recording; physics hands over geometry data, not draw calls.
