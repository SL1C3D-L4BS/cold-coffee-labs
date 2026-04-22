# 03 — Philosophy & Engineering Principles

**Status:** Canonical · Binding at review  
**Precedence:** L2 — values and review rubric; overridden only by `01`

> *"The Brew Doctrine is the soul of Greywater Engine. The specification is the skeleton. The architecture is the flesh. The daily work is the blood. Without the soul, the rest is a corpse."*

---

## Part I — The Brew Doctrine

### 1. What We Build and What We Don't

We do not build Greywater Engine to compete with the large commercial engines. Those products have ten-thousand-engineer head starts and are not going anywhere. Racing them on their own terms is a losing proposition.

We build Greywater Engine because the combination we need — a data-oriented native C++ runtime, a Rust-native AI copilot with authoritative tool-use, a text-IR-native visual scripting system that AI and humans coauthor, a Clang-only reproducible build on Windows and Linux, a Vulkan 1.2-baseline renderer designed for mid-range hardware, and deterministic procedural depth in service of *Sacrilege* and future titles — does not exist anywhere. Nobody is going to build it for us.

Nobody is asking for it either. That is why we are building it.

#### 1.1 Supra-AAA: the ceiling is not "licensed-engine blockbuster"

Industry AAA usually means high art spend on someone else's engine with a roadmap defined by risk committees. Supra-AAA is Cold Coffee Labs' bar: **sovereign stack**, **deterministic procedural depth**, **BLD-native authoring**, **RX 580–inclusive Tier A**, and **novel systems worth protecting** — combined in one program. We do not aim to match competitors; we aim to exceed their structural limits while shipping *Sacrilege* as the first proof.

---

### 2. Core Principles

#### 2.1 The engine exists to ship *Sacrilege* first

The engine is purpose-built for *Sacrilege*, the debut title Greywater Engine presents. *Sacrilege* drives the engine's feature priorities. We are not building a general-purpose engine that happens to ship a game — we are building the engine this debut title demands, with the discipline that it remains reusable for future Cold Coffee Labs titles via a clean engine/game code boundary.

#### 2.2 Infinite without storage

The universe does not exist as stored data. Everything is generated from a universe seed and spatial coordinates via deterministic procedural rule systems. Same seed + same coordinates = identical output, always.

**"If it is not near the player, it does not exist."** This is the foundational law of on-demand procedural simulation — whether arena-scale or universe-scale.

#### 2.3 Seamless presentation where the flagship demands it

*Sacrilege* is built on continuous, stutter-free traversal inside its arenas and circles. No cheap loading gates that break flow. The Blacklake stack can represent vast coherent spaces; the renderer and streaming systems must hold immersion at the target hardware budget (RX 580 baseline).

#### 2.4 Data-oriented design is the default

We do not design systems around objects and inheritance trees. We design them around data layout and access patterns. Our ECS is archetype-based. Our components are plain data. Our systems are functions over component spans. When a pattern would improve object-oriented ergonomics at the cost of cache locality, we reject the pattern.

**Cache-miss latency is the only budget that matters in the hot path.**

#### 2.5 Vulkan 1.2-baseline is a feature, not a limitation

Targeting Vulkan 1.2 with required 1.3 features is a commitment. It means every player on an RX 580 or better can run *Sacrilege* at 1080p / 144 FPS and the engine's full simulation at 1080p / 60 FPS. We do not exclude mid-range hardware to chase ray tracing or mesh shaders.

#### 2.6 The editor shares the engine's language, toolchain, and process

The editor is a native C++ executable that statically links `greywater_core`. Dear ImGui with docking and viewports. No managed runtime. No C# bridge. No Python interop. No custom DSL for tool widgets.

One debugger. One build. One language. One process.

#### 2.7 AI is not a plugin. It is a subsystem.

**BLD — the Brewed Logic Directive** — is a native AI copilot inside the engine. Implemented as a Rust crate linked into the editor via a narrow C-ABI. Not an editor plugin. Not a browser extension. Not a cloud IDE.

The engine's public C++ API is BLD's tool surface. The editor's command stack is BLD's commit log. MCP is the wire protocol from day one. Human-in-the-loop approval is protocol-native via MCP elicitation.

#### 2.8 Scripting is text-native and AI-native

Visual scripting in Greywater Engine is a reversible projection, not a ground-truth. The **typed text IR** is the serialized form. The node graph is rendered from the IR on open and re-serialized to IR on save.

Designers author by drag-and-drop. Programmers author by typing IR. BLD authors by generating IR. All three produce identical artifacts. There is no scripting VM, no Lua, no JavaScript, no embedded Python.

#### 2.9 Simulation depth over pixel density

We are not competing on photoreal skin shading. We are building an engine that produces **more simulation per engineer-hour** than what exists today. The mechanism is BLD. The leverage is architecture. The outcome is a smaller team shipping a bigger, more alive world.

#### 2.10 Determinism where it matters

Gameplay-critical systems — physics under lockstep, visual-script IR execution, asset cook, procedural universe generation — are deterministic. Same inputs, same outputs, across machines and platforms. Determinism is the contract that makes multiplayer rollback, replay, reproducible bug reports, and "infinite without storage" possible.

#### 2.11 The flagship defines the player fantasy

Every system exists to serve *Sacrilege* first — speed, violence, the Martyrdom loop, the Nine Inverted Circles, the tone in GW-SAC-001. If a subsystem does not make *Sacrilege* better, it does not get priority.

---

### 3. Aesthetic — Cold Drip × Sacrilege

#### 3.1 Editor and tools: Cold Drip

Restrained. Considered. Never ornamental. The UI is an engineering surface, not a visual product.

#### 3.2 UI Palette

| Token | Hex | Role |
|-------|-----|------|
| Obsidian | `#0B1215` | Primary background — editor chrome, HUD base |
| Greywater | `#6B7780` | Namesake accent — panels, dividers, secondary |
| Brew | `#5D4037` | Warm accent — branding, highlights |
| Crema | `#E8DDC8` | Text on dark — foam-like, warm |
| Signal | `#5865F2` | Interactive / action — buttons, focus ring |
| Bean | `#2A1F1A` | Deepest accent — shadows, panel depth |

**The palette is six colours.** Everything else is a semantic tint (success, warn, error, info) earned by meaning, not decorated in.

#### 3.3 Typography

- **Monospace (code, console, IR):** JetBrains Mono, Fira Code, or Cascadia Code. Ligatures on.
- **Sans (UI, docs):** Inter or IBM Plex Sans. Never system-default.
- **Serif:** only for long-form documentation on a rendered site.
- Two typefaces. Three weights max. A fixed size scale. No italic except for file paths and type names.

#### 3.4 Motion

- Under 150 ms for every meaningful animation.
- No bounce, no elastic, no ease-in-out-back. Ease-out cubic or linear.
- Respect `prefers-reduced-motion` — animations shorten to 0.

#### 3.5 Editor UI Doctrine (Ten Rules — Binding)

1. **Data foregrounds; chrome recedes.** The data should visually dominate its panel.
2. **One grid, one scale.** Every spacing value is a multiple of 4 px.
3. **Typography is a hierarchy, not a decoration.** Two typefaces, three weights max, fixed scale.
4. **Restraint over richness.** No gradients. No drop shadows. No skeuomorphic textures. No bevel.
5. **The palette is six colours.** Obsidian, Bean, Greywater, Crema, Brew, Signal. No more.
6. **Keyboard is first-class.** Every action has a shortcut. Ctrl+K command palette reaches everything.
7. **Docking, not floating.** Panels live in a dockspace. Floating is for multi-monitor, not the default.
8. **Workspaces exist.** Scene, materials, profiling, debugging — each loads with one keystroke.
9. **The agent has a seat at the table.** BLD is docked. Right-click offers "Ask BLD". This is not an afterthought.
10. **Performance is part of design.** UI renders in under 2 ms on RX 580. A panel that drops frames is broken.

---

### 4. What the Brew Doctrine Rules Out

- **No scripting VM.** No Lua, no JavaScript, no embedded Python. Text IR; hot-reloaded C++.
- **No custom UI framework.** ImGui + RmlUi, forever.
- **No Windows-first or Linux-first.** Both platforms are equal. Both enforced by CI on every PR.
- **No hand-waving AI.** BLD is not a marketing checkbox. Every BLD claim is backed by an architectural commitment.
- **No RTX-tier features that shut out RX 580 players.** Every Tier A visual feature runs at 1080p / 60 FPS on an RX 580.
- **No MSVC. No GCC.** Clang / LLVM only.
- **No Rust outside `bld/`.** The FFI seam is narrow and intentional.

---

### 5. The Signature

*Brewed slowly. Built deliberately. Shipped by Cold Coffee Labs. For I stand on the ridge of the universe and await nothing.*

If there is ever a question about what to do, ask: *is this brewed, or is this instant?*

Choose brewed.

---

## Part II — Engineering Principles (Code-Review Playbook)

> **Status:** Canonical · Binding at review.  
> A violation without a written justification blocks the merge.

### Why This Document Exists

Greywater Engine's codebase will outlive *Sacrilege* and every title after it. Code written in 2026 will be debugged in 2029 by engineers who weren't in the room. This playbook is the rubric applied at every pull request.

---

### Section A — Memory and Ownership

#### A1. No raw `new` / `delete` in engine code

**Rule.** Never write `new T(...)` or `delete ptr` in `engine/`, `editor/`, `sandbox/`, `runtime/`, `gameplay/` unless inside an allocator implementation or the one-line entry-point bootstrap.

**What to use instead:**

```cpp
// FORBIDDEN in engine code:
auto* buffer = new Buffer(size);
use(buffer);
delete buffer;

// REQUIRED:
auto buffer = std::make_unique<Buffer>(size);
use(*buffer);
// destructor runs at scope exit; no manual delete
```

- **Unique ownership:** `std::unique_ptr<T>`.
- **Shared, single-threaded:** `std::shared_ptr<T>` only when ownership is genuinely shared.
- **Shared, multi-threaded:** `gw::Ref<T>` (intrusive; avoids separate control block; see §C).
- **Cross-subsystem references:** `EntityHandle`, `TextureHandle`, `BufferHandle` — plain 64-bit values with generation.

**The one exception:** `main()` in `sandbox/main.cpp`, `editor/main.cpp`, `runtime/main.cpp` only.

#### A2. No two-phase construction / destruction on public APIs

**Rule.** Don't expose `init()` and `shutdown()` as mandatory public calls. If delayed init is required, express it structurally — `is_active()` predicate, protected `activate()` called by an owning class.

```cpp
// FORBIDDEN: public two-phase init
class Application {
public:
    Application(const AppSpec& spec);
    void init();      // caller must remember
    void shutdown();  // caller must remember
};

// PREFERRED: RAII
class Application {
public:
    Application(const AppSpec& spec);   // acquires everything
    ~Application();                      // releases everything
};
```

#### A3. Containers own their contents

```cpp
// FORBIDDEN:
class LayerStack {
    std::vector<Layer*> layers_;  // raw pointers, ownership unclear
};

// REQUIRED:
class LayerStack {
    std::vector<std::unique_ptr<Layer>> layers_;
public:
    void push(std::unique_ptr<Layer> layer) { layers_.push_back(std::move(layer)); }
};
```

#### A4. `unique_ptr` first; `shared_ptr` only when sharing is real

Use `unique_ptr` by default. Reach for `shared_ptr` / `gw::Ref<T>` only when multiple owners genuinely share a lifetime. Do not use `shared_ptr` as a way to avoid thinking about ownership.

#### A5. Pass shared ownership wisely

When a function borrows (not takes) a `shared_ptr`:

```cpp
void use_it(const std::shared_ptr<Texture>& tex);  // borrows: no ref-count bump
void store_it(std::shared_ptr<Texture> tex);        // takes: ref-count bump explicit
```

#### A6. Iterate `shared_ptr` / `Ref` containers by `const auto&`

```cpp
for (const auto& layer : layers_) {   // no ref-count bump per iteration
    layer->on_update();
}
```

#### A7. Move assignment must release existing resources

```cpp
// FORBIDDEN: leaks old_ resource
MyBuffer& operator=(MyBuffer&& other) {
    buffer_ = std::move(other.buffer_);
    return *this;
}

// REQUIRED: release first
MyBuffer& operator=(MyBuffer&& other) {
    if (this != &other) {
        release();                       // free existing
        buffer_ = std::move(other.buffer_);
    }
    return *this;
}
```

---

### Section B — Casts

#### B1. No C-style casts

```cpp
// FORBIDDEN:
auto* device = (VulkanDevice*)handle;

// REQUIRED:
auto* device = static_cast<VulkanDevice*>(handle);
```

#### B2. `static_cast` when the compiler can verify; `reinterpret_cast` only when it can't

`reinterpret_cast` appears in two places: the FFI boundary, and byte-level buffer reinterpretation. Everywhere else is a review-blocking smell.

#### B3. Reinterpreting bytes — know the UB exposure

```cpp
// FORBIDDEN: strict-aliasing UB
float f = ...;
uint32_t bits = *reinterpret_cast<uint32_t*>(&f);

// REQUIRED (C++23):
uint32_t bits = std::bit_cast<uint32_t>(f);

// Or for memory regions:
// SAFETY: buffer is aligned to alignof(T) and has lifetime >= this call
auto* typed = std::start_lifetime_as<T>(buffer.data());
```

---

### Section C — Smart Pointers and `gw::Ref`

#### C1. Prefer `gw::Ref<T>` over `std::shared_ptr<T>` in engine hot paths

`gw::Ref<T>` uses intrusive reference counting — the ref count lives inside the object, avoiding the separate control-block allocation `std::shared_ptr` requires.

#### C2. Intrusive ref counting is specifically for cross-thread lifetime

If the resource lives and dies on one thread, `unique_ptr` suffices. Reserve `gw::Ref<T>` for resources that genuinely cross thread lifetimes (textures shared between render thread and asset thread, audio streams, etc.).

#### C3. Cross-thread lambdas capture strong refs, not `this`

```cpp
// FORBIDDEN: this may be destroyed before the lambda runs
auto job = [this]() { render_(); };

// REQUIRED:
auto self = gw::ref(this);
auto job = [instance = self]() { instance->render_(); };
```

---

### Section D — Lambda Captures

#### D1. Capture narrowly — no `[=]`, no `[&]`

```cpp
// FORBIDDEN:
auto fn = [&]() { use(device_); use(pipeline_); };

// REQUIRED:
auto fn = [device = device_, pipeline = pipeline_]() {
    use(device);
    use(pipeline);
};
```

#### D2. Prefer `member = member_` over `this`-capture when only one field is needed

```cpp
// PREFERRED over capturing this just to access one member:
auto fn = [queue = queue_]() { queue->submit(...); };
```

---

### Section E — Strings and C APIs

#### E1. `std::string_view` is not null-terminated

```cpp
// FORBIDDEN — UB if view is not null-terminated:
void call_c_api(std::string_view sv) {
    fopen(sv.data(), "r");   // NOT safe
}

// REQUIRED:
void call_c_api(const std::string& s) {
    fopen(s.c_str(), "r");   // safe
}
```

#### E2. Use heterogeneous lookup in hash maps

```cpp
// FORBIDDEN: constructs std::string from the literal for the lookup:
std::unordered_map<std::string, Asset*> map;
auto it = map.find("texture_0");   // allocates

// REQUIRED: transparent hash
gw::core::HashMap<std::string, Asset*, gw::core::StringHash> map;
auto it = map.find(std::string_view{"texture_0"});   // no allocation
```

#### E3. Windows wide-string conversion uses the platform API

```cpp
// FORBIDDEN: assumes ASCII / breaks on non-ASCII:
std::wstring ws(path.begin(), path.end());

// REQUIRED:
std::wstring ws = platform::utf8_to_wide(path);  // MultiByteToWideChar(CP_UTF8, ...)
```

#### E4. Parse once, access many

No per-frame string parsing. Parse at load time; store the result; access the result.

---

### Section F — Events and Callbacks

#### F1. Do not heap-allocate transient event objects

```cpp
// FORBIDDEN: heap allocation per event:
event_bus.dispatch(new TextureLoadedEvent(handle));

// REQUIRED: stack + ref:
TextureLoadedEvent evt{handle};
event_bus.dispatch(evt);
```

#### F2. `std::function` may heap-allocate

For hot-path callbacks, use `std::move_only_function<void()>` (C++23) or a raw function pointer. Avoid `std::function` in engine work queues.

---

### Section G — Modern C++ Idioms

#### G1. Prefer `std::ranges` where it reads clearly

```cpp
// More expressive than a manual index loop:
auto alive = entities | std::views::filter([](const Entity& e) { return e.is_alive(); });
```

#### G2. `using enum` in switch (C++20)

```cpp
switch (state) {
    using enum RapturePhase;
    case Active:  apply_rapture();  break;
    case Ruin:    apply_ruin();     break;
    case None:    break;
}
```

---

### Section H — API Design

#### H1. Narrow public surface

Public functions are the contract. Private / protected helpers are implementation. Exposing implementation detail as public API makes every refactor a breaking change.

#### H2. Pure stateless helpers — free functions, not static-member classes

```cpp
// AVOID: the class adds nothing
class MathUtils {
public:
    static float clamp(float v, float lo, float hi);
};

// PREFER: a namespace
namespace gw::math {
    float clamp(float v, float lo, float hi) noexcept;
}
```

#### H3. Non-owning parameters: `const T&` or `T*`, not smart pointers

If a function borrows a resource but doesn't store it, take it by reference or raw pointer. Passing a `shared_ptr` by value implies shared ownership — signal that intent explicitly or don't.

#### H4. Thread-safety is part of the signature

```cpp
// REQUIRED annotation when thread policy is non-obvious:
/// @thread_safety Call only from the render thread (rt_ prefix enforces this).
void rt_flush_command_buffer();
```

---

### Section I — Concurrency

#### I1. No `std::async`, no raw `std::thread`

All concurrent work goes through `engine/jobs/`. The hot pattern:

```cpp
auto handle = jobs::submit("cook_texture", [src, dst]() {
    cook::texture_to_ktx2(src, dst);
});
handle.wait();

jobs::parallel_for(entities.size(), [&](size_t i) {
    update_transform(entities[i]);
});
```

#### I2. Thread affinity masks are bit masks, not integers

Document which thread a function runs on. Render-thread functions use the `rt_` prefix.

---

### Section J — Review Checklist

Before approving any PR, verify every item in your domain:

**Correctness**
- [ ] No `new` / `delete` in engine code (§A1)
- [ ] No two-phase init on public APIs (§A2)
- [ ] Containers own their contents (§A3)
- [ ] Move-assignment releases existing resources (§A7)
- [ ] No C-style casts (§B1)
- [ ] `reinterpret_cast` has a `// SAFETY:` comment (§B3)
- [ ] `string_view` never passed to a C API (§E1)
- [ ] Windows wide-string conversions use the platform API (§E3)

**Performance**
- [ ] `shared_ptr` / `gw::Ref` not copied unnecessarily (§A5)
- [ ] Range loops over ref containers use `const auto&` (§A6)
- [ ] No per-frame `std::string` allocation for load-time data (§E4)
- [ ] No `std::function` heap allocation on the hot path (§F2)

**Discipline**
- [ ] Lambda captures are explicit — no `[=]`, no `[&]` (§D1)
- [ ] Cross-thread lambdas capture strong refs, not `this` (§C3)
- [ ] Public API surface is minimal (§H1)
- [ ] Non-owning parameters are `const T&` or `T*` (§H3)
- [ ] Thread-safety is documented (§H4)

**Style**
- [ ] `clang-format`, `clang-tidy`, `rustfmt`, `clippy` all clean
- [ ] Names follow `01_CONSTITUTION_AND_PROGRAM.md §3`
- [ ] No commented-out code
- [ ] Conventional Commits format

**Rust (BLD)**
- [ ] No `unwrap`/`expect` in `bld-ffi` or `bld-governance`
- [ ] `unsafe` blocks have `// SAFETY:` comments
- [ ] FFI types are `#[repr(C)]`, no generics across the boundary
- [ ] Miri runs clean on any change to `bld-ffi`

---

### Section K — `engine/world/` / Blacklake / *Sacrilege* Rules

These rules are additive to Sections A–J for all code in `engine/world/` and for gameplay code touching Blacklake subsystems.

#### K1. World-space positions are `f64`. Period.

```cpp
// FORBIDDEN — breaks at planet scale:
struct Transform { Vec3 position; };        // f32

// REQUIRED:
struct Transform { Vec3_f64 position; ... } // f64
```

#### K2. Floating-origin events are mandatory for cached positions.

Subscribe to `world::origin_shift_event`. Translate cached positions when the event fires. Miss it and your subsystem silently diverges.

#### K3. Procedural generation is deterministic. No exceptions.

```cpp
// FORBIDDEN:
f32 height(Vec3_f64 dir) {
    std::mt19937 rng{std::random_device{}()};  // non-deterministic
    return noise(dir, rng());
}

// REQUIRED:
f32 height(Vec3_f64 dir, u64 seed) noexcept {
    std::mt19937_64 rng(seed);
    return noise(dir, rng());
}
```

No wall-clock reads. No `random_device`. No hardware-varying branches. Same seed + same coord = same output, always.

#### K4. Chunk generation must be time-sliced.

Hard cap: **10 ms per chunk step** on the main thread. Generation runs on the job system. Time-slice large chunks across multiple frames.

#### K5. Rollback-critical code is a pure function of game state.

No global state reads. No side effects except mutating the passed game state. No logging or telemetry inside the rollback window.

#### K6. Structural-integrity updates are deterministic and complete in one frame.

When a building piece changes, the integrity-graph recomputation finishes before the next frame renders. Partial recomputation leaves the graph inconsistent. Inconsistent graphs produce spurious collapses.

#### K7. The engine never hardcodes *Sacrilege* content.

No specific circle names, enemy proper names, weapon names, or boss names in `engine/world/`. Names and scenarios live in `gameplay/` data. The engine provides mechanisms; the title provides fiction.

---

*Reviewed deliberately. Every rule here is a scar. We honor the scar by not getting cut twice.*
