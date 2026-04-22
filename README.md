# Cold Coffee Labs — Greywater Engine

**Cold Coffee Labs** builds **Greywater Engine** (C++23 / Vulkan). Greywater Engine presents ***Sacrilege*** as the debut title and forcing function.

This repository is the monorepo for:

- **Greywater_Engine** — proprietary engine: ECS, jobs, Vulkan renderer, editor, `engine/world/`, tools.  
- ***Sacrilege*** — flagship game; program spec and vision in `docs/07_SACRILEGE.md`.  
- **BLD** (Brewed Logic Directive) — Rust copilot subsystem linked through a narrow C-ABI.

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
- *Brewed Logic* (Phase 9 / BLD + `.gwscene`) — engineering **complete**; ADR-0010–0016 + amended ADR-0007; six waves 9A–9F. Recording gate open.
- *Playable Runtime* (Phase 11) — **complete**; `playable_sandbox` exit gate. *Living Scene* (Phase 13) — **complete**; `living_sandbox`. *Two Client Night* (Phase 14) — **complete**; `netplay_sandbox`. *Ship Ready* (Phase 16) — **complete**; `sandbox_platform_services`.
- *Studio Renderer* (Phase 17) — engineering **complete**; ADR-0074–0082; `MaterialWorld` + `ParticleWorld` + `PostWorld`; `gw_perf_gate_phase17`; `apps/sandbox_studio_renderer` prints the `STUDIO RENDERER` exit marker; `dev-win` CTest **711/711** (see `docs/02_ROADMAP_AND_OPERATIONS.md` §Phase 17 and `docs/09_NETCODE_DETERMINISM_PERF.md` — merged **Phase 17 performance budgets — Studio Renderer**).

---

## Getting started

1. **Read** [`CLAUDE.md`](CLAUDE.md) — the cold-start primer for every session.
2. **Build** per [`docs/05_RESEARCH_BUILD_AND_STANDARDS.md`](docs/05_RESEARCH_BUILD_AND_STANDARDS.md) *(merged `BUILDING.md`)*.
3. **Read** [`docs/README.md`](docs/README.md) — map of the **eleven** Markdown files under `docs/`.

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

Engine code never `#include`s from `gameplay/`. The engine/game boundary is enforced in the review rubric (`docs/03_PHILOSOPHY_AND_ENGINEERING.md` §K).

---

## The canonical set (binding)

The `docs/` folder is intentionally **eleven Markdown files** (see [`docs/README.md`](docs/README.md)).

| | |
| --- | --- |
| [`CLAUDE.md`](CLAUDE.md) | Cold-start primer + non-negotiables |
| [`docs/01_CONSTITUTION_AND_PROGRAM.md`](docs/01_CONSTITUTION_AND_PROGRAM.md) | Constitution, executive brief, handoff |
| [`docs/02_ROADMAP_AND_OPERATIONS.md`](docs/02_ROADMAP_AND_OPERATIONS.md) | Roadmap, milestones, Kanban |
| [`docs/03_PHILOSOPHY_AND_ENGINEERING.md`](docs/03_PHILOSOPHY_AND_ENGINEERING.md) | Brew doctrine + engineering principles |
| [`docs/04_SYSTEMS_INVENTORY.md`](docs/04_SYSTEMS_INVENTORY.md) | Subsystem inventory |
| [`docs/05_RESEARCH_BUILD_AND_STANDARDS.md`](docs/05_RESEARCH_BUILD_AND_STANDARDS.md) | Vulkan/C++/Rust guides, build, coding standards |
| [`docs/06_ARCHITECTURE.md`](docs/06_ARCHITECTURE.md) | Blueprint + core architecture (merged) |
| [`docs/07_SACRILEGE.md`](docs/07_SACRILEGE.md) | Flagship program spec |
| [`docs/08_BLACKLAKE_AND_RIPPLE.md`](docs/08_BLACKLAKE_AND_RIPPLE.md) | Blacklake + GWE × Ripple studio specs |
| [`docs/09_NETCODE_DETERMINISM_PERF.md`](docs/09_NETCODE_DETERMINISM_PERF.md) | Netcode, determinism, perf annexes |
| [`docs/10_APPENDIX_ADRS_AND_REFERENCES.md`](docs/10_APPENDIX_ADRS_AND_REFERENCES.md) | Glossary, appendix, **all ADRs** |

---

## Studio

**Cold Coffee Labs** — founder: <slicedlabs.founder@proton.me>

*Brewed slowly. Built deliberately. Shipped by Cold Coffee Labs.*
