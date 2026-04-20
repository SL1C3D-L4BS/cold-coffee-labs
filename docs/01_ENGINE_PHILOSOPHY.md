# 01_ENGINE_PHILOSOPHY — The Brew Doctrine (Greywater Edition)

**Status:** Canonical
**Author:** Cold Coffee Labs
**Date:** 2026-04-19

*"For I stand on the ridge of the universe and await nothing."*

---

This document is the values statement of the Greywater project — both the engine (`Greywater_Engine`) and the game it powers (`Greywater`). It is not a specification. It is the posture from which every specification is written. When a technical question has more than one reasonable answer, we resolve it by asking: which choice is most aligned with the Brew Doctrine?

---

## 1. What we build and what we don't

We do not build Greywater_Engine to compete with the large commercial engines. Those products have ten-thousand-engineer head starts and they're not going anywhere. Racing them on their own terms is a losing proposition we have no interest in.

We build Greywater because the combination we want to ship — **a data-oriented native C++ runtime, a Rust-native AI copilot with authoritative tool-use over the public API, a text-IR-native visual scripting system that AI and humans coauthor, a Clang-only reproducible build across Windows and Linux, a Vulkan 1.2-baseline renderer designed for mid-range hardware (including the RX 580), and a persistent procedural universe simulation that unifies ground-to-orbit traversal, survival gameplay, and a living ecosystem** — is not available anywhere. Nobody is going to build it for us.

Nobody is asking for it either. That is why we are building it.

---

## 2. Core principles

### 2.1 The engine exists to power Greywater

The engine is purpose-built for the game. The game drives the engine's feature priorities. We are not building a general-purpose engine that happens to ship a game — we are building **the engine Greywater needs**, with the discipline that it remains reusable for future Cold Coffee Labs titles via a clean engine/game code boundary.

### 2.2 Infinite without storage

The universe does not exist as stored data. Everything is generated from a **universe seed** and **spatial coordinates** via deterministic procedural rule systems. Same seed + same coordinates = identical output, always.

**"If it is not near the player, it does not exist."** This is the foundational law of Greywater's world.

### 2.3 Seamless simulation continuity

There are no loading screens, no hard transitions, no "you have entered a new zone" moments. **Ground → Atmosphere → Orbit → Interplanetary** is one continuous simulation. The player takes off, ascends through five atmospheric layers, feels the air thin, watches the sky fade from blue to black, and reaches orbit — all in one unbroken experience.

### 2.4 Data-oriented design is the default

We do not design systems around objects and inheritance trees. We design them around data layout and access patterns. Our ECS is archetype-based. Our components are plain data. Our systems are functions over component spans. When a pattern would improve object-oriented ergonomics at the cost of cache locality, we reject the pattern.

Cache-miss latency is the only budget that matters in the hot path.

### 2.5 Vulkan 1.2-baseline is a feature, not a limitation

Targeting Vulkan 1.2 with opportunistic 1.3 features is a commitment. It means **every player on an RX 580 or better can run Greywater at 1080p / 60 FPS.** We don't exclude mid-range hardware to chase ray tracing or mesh shaders.

Bindless descriptors, timeline semaphores, dynamic rendering, synchronization-2, buffer device address — all Vulkan 1.3 features that the RX 580 supports — are baseline. Ray tracing, mesh shaders, GPU work graphs are capability-gated and **not required** for a complete experience.

Vulkan 1.4 features land behind capability flags in `RHICapabilities`. WebGPU support is a planned second backend for web and sandboxed targets, not a concession to lowest-common-denominator design.

### 2.6 The editor shares the engine's language, toolchain, and process

The editor is a native C++ executable that statically links `greywater_core`. It uses Dear ImGui with docking and viewports. There is no managed runtime, no C# bridge, no Python interop, no custom DSL for tool widgets. Anything a gameplay programmer can do, a tools programmer can do, and vice versa.

One debugger. One build. One language. One process.

### 2.7 AI is not a plugin. It is a subsystem.

**BLD — the Brewed Logic Directive** — is a native AI copilot inside the engine. It is implemented as a Rust crate linked into the editor via a narrow C-ABI. It is **not** an editor plugin, a browser extension, a managed-process sidecar, or a cloud IDE pretending to be an IDE.

The engine's public C++ API is BLD's tool surface. The editor's command stack is BLD's commit log. MCP is the wire protocol from day one. Human-in-the-loop approval is protocol-native (MCP elicitation). A shipping Greywater build can embed BLD for in-game modding and debug tooling — not because we want to, because we designed it to be embeddable from the first commit.

### 2.8 Scripting is text-native and AI-native

Visual scripting in Greywater is a reversible projection, not a ground-truth. The **typed text IR** is the serialized form. The node graph is rendered from the IR on open and re-serialized to IR on save, with a sidecar file remembering node positions.

This is the architectural payoff of treating AI as a first-class user. The representation that is easiest for BLD to read and write — structured text — is also the representation that diffs cleanly in source control, merges without binary-blob pain, and survives refactoring. Designers author by drag-and-drop; programmers author by typing IR; BLD authors by generating IR. All three produce identical artifacts.

### 2.9 Simulation depth over pixel density

We are not competing on photoreal skin shading. We are not shipping a planet-scale asset library built by another studio's pipeline. We are not publishing a global-illumination-of-the-year paper.

We are building an engine that produces **more simulation per engineer-hour** than what exists today. The mechanism is BLD. The leverage is architecture. The outcome is a smaller team shipping a bigger, more alive world.

### 2.10 Determinism where it matters

Gameplay-critical systems — physics under lockstep, visual-script IR execution, asset cook, procedural universe generation — are deterministic. Same inputs, same outputs, across machines and platforms. Determinism is not a "nice-to-have"; it is the contract that makes multiplayer rollback, replay, reproducible bug reports, and the "infinite without storage" principle possible.

### 2.11 Survival is the center of everything

Greywater's core gameplay loop is **Gather → Craft → Build → Raid → Dominate → Survive.** Every system in the simulation supports conflict and survival. Resources are scarce. Territory must be claimed. Bases must be built, defended, and can be raided. The universe is hostile by design.

---

## 3. Aesthetic — *Cold Drip × Realistic Low-Poly*

The visual identity of Greywater expresses the Brew Doctrine directly. Restrained, considered, never ornamental. And — specifically for the game's worlds — **realistic low-poly**.

### 3.1 Realistic low-poly (art direction)

Not blocky voxel. Not photoreal. The seven properties of Greywater's visual style:

- **Reduced polygon counts** on meshes. Geometry is deliberate, not maximal.
- **Flat shading or subtle-gradient shading** as the default. Specular and normal detail are restrained.
- **Realistic lighting.** Physically-based rendering for the shading model; atmospheric scattering, real shadow cascades, IBL for ambient. Lighting is where we invest visual budget.
- **Strong color palettes per biome and planet type.** Limited palettes per scene; saturation controlled by atmospheric composition.
- **Clean silhouettes.** Fewer polys means each polygon matters — silhouettes are authored carefully.
- **Atmospheric scattering drives mood.** A desert planet's orange sky, an ice world's blue shadows, a toxic world's sickly green horizon — the atmosphere is the principal mood lever.
- **Material restraint.** Mid-range roughness, low metalness by default. No chrome, no glossy plastic, no wet-puddle highlights on otherwise-matte surfaces.

Full art-direction specification lives in `docs/games/greywater/20_ART_DIRECTION.md`.

### 3.2 UI palette

| Token       | Hex        | Role                                          |
| ----------- | ---------- | --------------------------------------------- |
| Obsidian    | `#0B1215`  | Primary background — editor chrome, HUD base |
| Greywater   | `#6B7780`  | Namesake accent — panels, dividers, secondary |
| Brew        | `#5D4037`  | Warm accent — branding, highlights            |
| Crema       | `#E8DDC8`  | Text on dark — foam-like, warm                |
| Signal      | `#5865F2`  | Interactive / action — buttons, focus ring    |
| Bean        | `#2A1F1A`  | Deepest accent — shadows, panel depth         |

### 3.3 Typography

- **Monospace (code, console, IR):** JetBrains Mono, Fira Code, or Cascadia Code. Ligatures on.
- **Sans (UI, docs):** Inter or IBM Plex Sans. Never system-default.
- **Serif:** only for long-form documentation reading on a rendered site.

### 3.4 Iconography

- **Line icons only.** Lucide or custom. No filled icons, no gradient icons, no skeuomorphic icons.
- **Consistent weight.** 1.5 px strokes at 16 px nominal.

### 3.5 Motion

- **Under 150 ms** for every meaningful animation.
- **No bounce, no elastic, no ease-in-out-back.** Ease-out cubic or linear.
- **Respect motion-reduction** accessibility preferences — animations shorten to 0 on reduced-motion.

### 3.6 The editor UI doctrine

The editor is an engineering surface, not a visual product. The UI is a tool that gets out of the way so the work can land.

**Ten doctrinal rules for the editor UI** — these are binding for every panel we build:

1. **Data foregrounds; chrome recedes.** The data an engineer is looking at should visually dominate its panel. Panel borders, labels, and controls are secondary, coloured one step closer to the background.
2. **One grid, one scale.** Every spacing value is a multiple of 4 px. Every font size is on a defined scale. Off-grid or off-scale values are a review-blocking smell.
3. **Typography is a hierarchy, not a decoration.** Two typefaces (one sans, one mono). Three weights max. A fixed scale of sizes. No italic except for file paths and type names. No underline except on links.
4. **Restraint over richness.** No gradients, no drop shadows, no skeuomorphic textures, no bevel. Flat panels, flat widgets, 1 px dividers, rounded corners only where interactive affordance benefits from them.
5. **The palette is six colours.** Obsidian, Bean, Greywater, Crema, Brew, Signal. Everything else is a semantic tint (success, warn, error, info) earned by meaning, not decorated in.
6. **Keyboard is first-class.** Every action has a shortcut. The command palette (Ctrl+K) reaches every action. The mouse is for pointing, not for discovering.
7. **Docking, not floating.** Panels live in a dockspace. Floating windows are detachable for multi-monitor, not the default. The default layout is one coherent wall of tools.
8. **Workspaces exist.** Different tasks need different layouts. Scene editing, materials, profiling, debugging — each is a workspace preset that loads with one keystroke.
9. **The agent has a seat at the table.** BLD is docked. Right-click on any selection offers "Ask BLD". This is an editor feature, not an afterthought.
10. **Performance is part of design.** The UI renders in under 2 ms on RX 580. A panel that drops frames to render itself is broken, regardless of how pretty it looks.

Formal specification of the editor UI framework — panels, widgets, layout, interaction patterns, performance targets — lives in `docs/architecture/grey-water-engine-blueprint.md` §3.31 and `docs/architecture/greywater-engine-core-architecture.md` Chapter 13.

---

## 4. What the Brew Doctrine rules out

To define a posture is to reject its opposites. We are clear about what we will not do.

- **We will not add a scripting VM** (no Lua, no JavaScript, no embedded Python). Visual scripting is a typed text IR; gameplay extension is hot-reloaded C++.
- **We will not ship a "visual programming for non-programmers" narrative.** Anyone authoring logic is a programmer of some kind. The tools respect that.
- **We will not build our own UI framework.** ImGui + RmlUi, forever.
- **We will not cater to Windows-first or Linux-first.** Both are equal and both are enforced by CI on every PR.
- **We will not hand-wave AI.** BLD is not a marketing checkbox. Every claim about BLD is backed by an architectural commitment in the blueprint.
- **We will not optimize for engine-marketing screenshots.** Screenshots follow from the engine; the engine does not follow from screenshots.
- **We will not chase RTX-tier features that shut out RX 580 players.** Every Tier A visual feature must run at 1080p @ 60 FPS on an RX 580.
- **We will not pretend the universe is infinite.** It is procedurally generated and effectively unbounded, but we acknowledge the mathematical and hardware constraints that shape it.

---

## 5. The signature

**Brewed slowly. Built deliberately. Shipped by Cold Coffee Labs. For I stand on the ridge of the universe and await nothing.**

If there is ever a question about what to do, ask: *is this brewed, or is this instant?*

Choose brewed.

---

*The Brew Doctrine is the soul of Greywater. The specification is the skeleton. The architecture is the flesh. The daily work is the blood. Without the soul, the rest is a corpse.*
