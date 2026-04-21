# Cold Coffee Labs — Greywater

**Greywater** — a persistent procedural universe simulation, shipping on its own proprietary engine.

This repository is the monorepo for:

- **Greywater_Engine** — a C++23 engine built from first principles on 2026 engineering standards.
- **Greywater** (the game) — a persistent procedural universe simulation with seamless ground-to-orbit traversal, PvP survival, base building, and a living AI ecosystem. The first title shipping on Greywater_Engine.
- **BLD** (Brewed Logic Directive) — a native AI copilot subsystem written in Rust, linked into the editor via a narrow C-ABI.

**Hardware baseline:** AMD Radeon RX 580 8GB @ 1080p / 60 FPS. Every Tier A feature runs there.
**Languages:** C++23 (engine) · Rust stable, 2024 edition (BLD only).
**Compiler:** Clang/LLVM 18+ (clang-cl on Windows, clang on Linux). MSVC and GCC are not supported.
**Build:** CMake 3.28+ with Presets + Ninja · Corrosion for Cargo ↔ CMake · CPM.cmake for pinned C++ deps.
**Rendering:** Vulkan 1.2 baseline; 1.3 features opportunistic via `RHICapabilities`.
**UI:** Dear ImGui + docking + viewports + ImNodes + ImGuizmo (editor only) · RmlUi (in-game).
**Platforms:** Windows + Linux from day one. macOS / consoles / mobile are post-scope.

*"For I stand on the ridge of the universe and await nothing."*

**Current milestone status (2026-04-21):**
- *First Light* (Phase 1) — engineering **complete**; recording gate open.
- *Foundation Renderer* (Phase 5) — engineering **complete** (`b191ca4`); recording gate open.
- *Foundations Set* (Phase 3) — engineering **complete** (pulled forward through Phase 7 per the 2026-04-20 audit; 101/101 doctest suite green, including the generational-handle ABA guard fixed today).
- *Editor v0.1* (Phase 7) — engineering **complete** across five commit gates (A–E). Gate E wired a cockpit-polish UI inspired by the Cold Coffee Labs reference mock-up: Scene stats, Render Settings (tone-map + SSAO + exposure + histogram), Lighting, and Render Targets, all bound to a shared `editor::render::RenderSettings`. Recording gate open.
- *Brewed Logic* (Phase 8 / `.gwscene` binary format) — **in progress**.

---

## Getting started

1. **Read** [`CLAUDE.md`](CLAUDE.md) — the cold-start primer for every session.
2. **Build** per [`docs/BUILDING.md`](docs/BUILDING.md).
3. **Read** [`docs/README.md`](docs/README.md) for the full document hierarchy.

```bash
# Linux
cmake --preset dev-linux
cmake --build --preset dev-linux
./build/dev-linux/bin/gw_sandbox   # hello-triangle (Phase 1 First Light)
./build/dev-linux/bin/gw_editor    # BLD C-ABI greeting
ctest --preset dev-linux

# Windows (clang-cl; no Visual Studio required)
cmake --preset dev-win
cmake --build --preset dev-win
.\build\dev-win\bin\gw_sandbox.exe
```

---

## Repository layout

```
engine/       Greywater_Engine core (static library, C++23)
editor/       Native C++ editor (statically links greywater_core + BLD)
sandbox/      Engine feature test bed (First Light milestone lives here)
runtime/      Shipping runtime executable
gameplay/     Hot-reloadable gameplay C++ dylib
bld/          BLD — Rust workspace with 8 crates (MCP, tools, provider, RAG, agent, governance, bridge, ffi)
tools/cook/   gw_cook content cooker CLI
tests/        doctest unit + integration + perf + fuzz
assets/       Cooked deterministic runtime assets (tracked)
content/      Source assets (gitignored)
cmake/        CPM.cmake, dependencies.cmake, toolchains, packaging
docs/         Canonical + operational documentation (read docs/README.md)
.github/      CI workflows (matrix + sanitizer nightly + Miri)
```

Engine code never `#include`s from `gameplay/`. The engine/game boundary is enforced in the review rubric (`docs/12_ENGINEERING_PRINCIPLES.md §K7`).

---

## The canonical set (binding)

| | |
| --- | --- |
| [`CLAUDE.md`](CLAUDE.md) | Cold-start primer + non-negotiables |
| [`docs/00_SPECIFICATION_Claude_Opus_4.7.md`](docs/00_SPECIFICATION_Claude_Opus_4.7.md) | Constitution — constraints, authorization, execution cadence |
| [`docs/01_ENGINE_PHILOSOPHY.md`](docs/01_ENGINE_PHILOSOPHY.md) | The Brew Doctrine + realistic low-poly art direction |
| [`docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md`](docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md) | 90-day sprint plan, first-week scaffolding |
| [`docs/12_ENGINEERING_PRINCIPLES.md`](docs/12_ENGINEERING_PRINCIPLES.md) | Code-review playbook |
| [`docs/architecture/grey-water-engine-blueprint.md`](docs/architecture/grey-water-engine-blueprint.md) | Formal architectural blueprint |
| [`docs/architecture/greywater-engine-core-architecture.md`](docs/architecture/greywater-engine-core-architecture.md) | Narrative core architecture |

---

## Studio

**Cold Coffee Labs** — founder: <slicedlabs.founder@proton.me>

*Brewed slowly. Built deliberately. Shipped by Cold Coffee Labs.*
