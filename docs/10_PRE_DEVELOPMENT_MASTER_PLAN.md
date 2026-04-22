# 10_PRE_DEVELOPMENT_MASTER_PLAN — Greywater_Engine

**Status:** Canonical · Binding for the first 13 weeks
**Horizon:** First 90 days (Phase 1 + Phase 2 + first four weeks of Phase 3, within the 24-phase roadmap plus LTS)
**Date:** 2026-04-19

This is the plan that drives the first three months of Greywater_Engine. Everything after week 13 is governed by `05_ROADMAP_AND_MILESTONES.md`. What you find here is the **design crystallization** plus **ready-to-compile code generation** for week-one scaffolding.

---

## 1. Executive summary & technical viability

Greywater_Engine is technically viable on 2026 infrastructure. The three risks most likely to sink the project, in order:

### Risk #1 — Solo/small-team burnout

**Likelihood:** High. **Impact:** Catastrophic. **Mitigation:**

- Kanban WIP limit of 2 is hard-enforced (`06_KANBAN_BOARD.md`).
- Daily log discipline → explicit single-focus blocks.
- Two mandatory rest days per month.
- 30-minute Sunday retro with **no coding**.
- Milestones are gated by recorded demos, not by "it compiles" — forcing the team to experience the work as users.

### Risk #2 — FFI boundary tax on BLD

**Likelihood:** Medium. **Impact:** Medium (bounded by design). **Mitigation:**

- The C-ABI between Rust and C++ is narrow and stable. Every change requires an ADR.
- Miri runs nightly on `bld-ffi` to catch UB.
- Tool-call frequency is low (dozens/minute) and payloads are already serialized (MCP JSON) — the FFI tax does not land in the per-frame hot path.
- If Rust ecosystem shifts in 2027, the boundary is narrow enough to swap BLD's language with a defined effort (~4 weeks).

### Risk #3 — Hardware / driver divergence across Vulkan vendors

**Likelihood:** Medium. **Impact:** Medium. **Mitigation:**

- Validation layers on in debug; zero tolerance for warnings in release-candidate build.
- Reference hardware list is published; exit criteria are measured on it.
- RenderDoc captures archived for every milestone.
- `RHICapabilities` gates optional features; code paths conditionally adapt without runtime branching in shaders.

All three risks have *structural* mitigations — baked into workflow and architecture, not into "remember to do this."

---

## 2. Critical design decisions (crystallized)

The following are not up for re-argument during the build. They are captured here so a reader understands the *why* behind the scaffolding.

### 2.1 Renderer — custom HAL over Vulkan 1.3

**Decision.** Hand-written HAL in `engine/render/hal/` with a Vulkan 1.3 backend. WebGPU post-v1.

**Why.** `wgpu` is shaped for the WebGPU intersection of APIs and costs us modern-Vulkan features. `ash` is the same as hand-rolling in C++ (same unsafe API, different syntax). Hand-rolling in C++ keeps the engine-core dependency graph clean.

**Cost we accept.** ~4 months of HAL + renderer implementation work (Phases 4–5).

### 2.2 ECS — in-house, archetype-based

**Decision.** Custom `engine/ecs/`. Reference EnTT/flecs for design but do not depend on them.

**Why.** The ECS is the most load-bearing subsystem. Owning it end-to-end is strategically correct. Also: the ECS is where BLD's tool surface is densest (`component.*`, `scene.*`) — we want to control reflection and schema evolution.

**Cost.** Higher risk and longer Phase 3. Mitigated by keeping design simple and referencing proven patterns.

### 2.3 Language split — C++23 engine, Rust BLD only

**Decision.** Rust nowhere except the BLD crate. The rest of the codebase is C++23.

**Why.** Three concrete Rust wins in BLD: proc-macro tool surfaces > libclang codegen, `rmcp` > any C++ MCP SDK, `candle`/`llama-cpp-rs` > piecing together libcurl + simdjson. Everywhere else, Rust loses (renderer, editor ecosystem, AAA precedent, hiring pool, template metaprogramming, hot-reload).

**Cost.** FFI boundary maintenance. Mitigated by keeping the boundary narrow.

### 2.4 UI stack — locked ImGui + RmlUi

**Decision.** Editor UI is Dear ImGui + docking + viewports + ImNodes + ImGuizmo, **locked**. In-game UI is RmlUi. No custom UI framework.

**Why.** Writing a UI framework is an 18-36 month distraction with no product differentiation. Our product is the engine, not the UI. ImGui is the correct answer for engine editors in 2026 and will still be correct in 2030.

**Cost.** RmlUi has a smaller community than commercial UI. Mitigated by scoping UI tightly and contributing upstream when we find bugs.

### 2.5 Visual scripting — text IR ground-truth

**Decision.** A typed text IR is canonical; the node graph is a reversible projection with a sidecar layout file.

**Why.** BLD can read and write text. Humans can diff and merge text. Blueprint-style graph-as-ground-truth doesn't merge and forces nativization. We avoid both problems.

**Cost.** IR design in Phase 8 is slightly slower than "just use the graph." Recouped every time a merge doesn't conflict.

### 2.6 Job system — custom fiber-based

**Decision.** Custom work-stealing fiber scheduler (`engine/jobs/`). Fibers preserve stack state across yields; worker threads pull from work-stealing deques; fibers let long-running tasks suspend without thread explosion.

**Why.** Fibers preserve stack across yields, critical for long-running tasks (asset cook, RAG indexing, physics step). `std::async` and `std::thread` cannot express this. Third-party (`marl`, TBB) either constrains design or adds weight.

**Cost.** Custom fiber primitives on Linux need validation — `makecontext`/`swapcontext` have ABI pitfalls. Mitigated by a small platform-specific shim and focused tests.

### 2.7 Hot-reload scope

**Decision.** Only the C++ gameplay module hot-reloads. The Rust BLD crate and the engine core do not.

**Why.** Rust hot-reload is hard (monomorphization, trait-object layout). Engine-core hot-reload would require dynamic linking through the entire core — too invasive. Gameplay module hot-reload covers the 90 % case (gameplay iteration).

**Cost.** Engine-core changes require a restart to see. Acceptable.

### 2.8 Memory policy

**Decision.** No global `new`/`delete` in engine code. No STL containers in hot paths. Allocators passed explicitly.

**Why.** Allocation discipline is a performance and correctness property. STL container allocator customization is painful; bespoke containers are cleaner.

**Cost.** Engineers coming from STL-heavy codebases need an onboarding ramp. Mitigated by `engine/core/` exposing familiar-shaped APIs (`span`, `vector`, `hash_map`) with allocator-first signatures.

---

## 3. 90-day sprint plan (weeks 01–13)

Covers Phase 1 (scaffolding, 4w) + Phase 2 (Platform & Core Utilities, 5w) + first 4 weeks of Phase 3 (Math, ECS, Jobs & Reflection). HAL bootstrap begins in Phase 4 at week 17 — outside this 13-week horizon.

> **Cadence note (binding).** The Monday-through-Friday breakdown below is a **pace budget**, not a per-day gate. When Phase 1 opens, all Phase 1 deliverables are in-scope simultaneously (see `00_SPECIFICATION_Claude_Opus_4.7.md` §9, `CLAUDE.md` non-negotiable #19, `06_KANBAN_BOARD.md §Phase entry`). The per-day breakdown tells you the order the author expected things to happen in — not "what is allowed to land today." A session that ships Monday's, Tuesday's, and Wednesday's items together is not violating anything; it is executing the phase. The gate is the phase milestone (*First Light* at week 004 for Phase 1), not the week row.

### Week 01 — Repository & build skeleton

**Pull card:** `P1.01` (`chore: scaffold repo and CMake root`).

Monday:
- Run `docs/bootstrap_command.txt` to create the directory tree.
- Commit the empty skeleton: `feat(scaffold): initialize engine/editor/sandbox/runtime/gameplay/bld trees`.
- Write root `CMakeLists.txt` (see §6.1 below).
- Write `CMakePresets.json` (see §6.2 below).

Tuesday:
- Integrate CPM.cmake: add `cmake/dependencies.cmake`.
- Pin first four C++ dependencies: `glfw`, `volk`, `VulkanMemoryAllocator`, `doctest`. Commit SHAs in `cmake/dependencies.cmake`.
- Write `cmake/toolchains/clang-cl.cmake` for Windows and `cmake/toolchains/clang.cmake` for Linux.

Wednesday:
- Set up `bld/` Cargo workspace with stub crates: `bld-mcp`, `bld-tools`, `bld-provider`, `bld-rag`, `bld-agent`, `bld-governance`, `bld-bridge`, `bld-ffi`. Each has a minimal `Cargo.toml` and `lib.rs` with one test.
- Write `rust-toolchain.toml` pinning stable.
- Integrate Corrosion into the root CMake: `find_package(Corrosion REQUIRED)` and `corrosion_import_crate(MANIFEST_PATH bld/Cargo.toml)`.

Thursday:
- Write `.github/workflows/ci.yml` with the matrix: `{windows-latest, ubuntu-latest} × {debug, release}`.
- Steps: checkout → install Vulkan SDK → `cmake --preset {os}-{config}` → `cmake --build` → `ctest`.
- Add `cargo build --release` and `cargo test` for the Rust side.

Friday:
- Write `.clang-format`, `.clang-tidy`, `rustfmt.toml`, `.editorconfig`, `.gitattributes`.
- Open first PR; land with green CI on both OSes and both configs.

Exit criteria:
- `cmake --preset dev-linux && cmake --build --preset dev-linux` produces an empty but linking set of artifacts on Linux.
- Same on Windows with `dev-win`.
- CI green on `main`.

### Week 02 — Hello triangle + hello BLD

**Pull card:** `P1.02` (`feat: hello-triangle sandbox end-to-end`).

Implementation:
- `engine/platform/window.hpp` + GLFW wrapper.
- `engine/render/hal/device.hpp` — just enough to create a `VkInstance` and pick a physical device.
- `sandbox/main.cpp` — creates a window, a Vulkan device, a swapchain, records a command buffer that clears the screen.
- **Milestone 1A:** The triangle.
- `bld/bld-ffi/src/lib.rs` — exposes one function `bld_hello() -> *const c_char` returning a compiled-in greeting.
- `editor/bld_bridge/hello.cpp` — loads the greeting via `cxx`, prints to stdout.
- **Milestone 1B:** Hello BLD C-ABI smoke test.

Exit criteria (*First Light* milestone):
- `sandbox/gw_sandbox` renders a triangle on both Windows and Linux.
- `editor/gw_editor` prints the BLD greeting on both.
- Record two-minute video; commit to `docs/milestones/first_light.mp4` (or external link if binary storage is off-repo).

### Weeks 03–04 — Scaffolding polish

Card set:
- `P1.03` — `docs(standards): write CODING_STANDARDS.md derived from 00 + 04`.
- `P1.04` — `chore(ci): enable sanitizer matrix nightlies (ASan/UBSan/TSan/Miri)`.
- `P1.05` — `chore(ci): wire sccache on Windows and Linux runners`.
- `P1.06` — `docs(build): write BUILDING.md with onboarding steps`.

Exit criteria for Phase 1:
- A fresh clone on a fresh machine on either OS builds and runs both milestones via a single preset invocation.
- CI is green; blocking required-check set is configured on `main`.

### Week 05 — Platform layer (Phase 2)

**Pull cards:** `P2.01`–`P2.03`.

- `engine/platform/window.cpp` (complete GLFW wrap)
- `engine/platform/input.cpp` (keyboard + mouse baseline; SDL3 gamepad in Phase 10)
- `engine/platform/time.cpp` (high-resolution clock)
- `engine/platform/fs.cpp` (filesystem abstraction, virtual-mount prep)
- `engine/platform/dll.cpp` (dynamic-library load for hot-reload)

### Week 06 — Memory allocators (Phase 2)

**Pull cards:** `P2.04`–`P2.06`.

- `engine/memory/frame_allocator.hpp/cpp` (linear, bump, reset per frame)
- `engine/memory/arena_allocator.hpp/cpp` (stack-like, scoped)
- `engine/memory/pool_allocator.hpp/cpp` (fixed-size free list)
- `engine/memory/freelist_allocator.hpp/cpp`
- Unit tests with ASan on; zero leaks, zero UB.

### Week 07 — Core utilities (Phase 2)

**Pull cards:** `P2.07`–`P2.10`.

- `engine/core/log.hpp/cpp`
- `engine/core/assert.hpp/cpp`
- `engine/core/result.hpp` (`Result<T, E>`)
- `engine/core/error.hpp` (error taxonomy)

### Week 08 — Containers (Phase 2)

**Pull cards:** `P2.11`–`P2.15`.

- `engine/core/span.hpp`
- `engine/core/array.hpp`
- `engine/core/vector.hpp` (allocator-first signature)
- `engine/core/hash_map.hpp` (heterogeneous-lookup friendly)
- `engine/core/ring_buffer.hpp`

### Week 09 — Strings, OS-header lint (Phase 2 exit)

**Pull cards:** `P2.16`–`P2.18`.

- `engine/core/string.hpp/cpp` (UTF-8 with small-string optimisation)
- Custom `clang-tidy` check `gw-no-os-headers-outside-platform` authored and enabled.
- Phase 2 exit gate review.

### Week 10 — Math library (Phase 3 start)

**Pull cards:** `P3.01`–`P3.04`.

- `engine/math/vec.hpp` (Vec2, Vec3, Vec4 with SIMD-capable storage)
- `engine/math/mat.hpp` (Mat3, Mat4)
- `engine/math/quat.hpp`
- `engine/math/transform.hpp`
- SIMD path behind `GW_ENABLE_SIMD`; mainline correct without.

### Week 11 — Command bus (Phase 3)

**Pull cards:** `P3.05`–`P3.06`.

- `engine/core/command/command.hpp` (`ICommand`)
- `engine/core/command/stack.hpp/cpp` (`CommandStack` with transaction brackets)
- External registration API exposed via opaque function pointers (for BLD C-ABI in Phase 9).

### Week 12 — Jobs skeleton (Phase 3)

**Pull cards:** `P3.07`–`P3.09`.

- `engine/jobs/fiber.hpp/cpp` (platform-specific fiber primitives — `makecontext`/`swapcontext` on Linux, Fiber API on Windows)
- `engine/jobs/scheduler.hpp/cpp` (work-stealing deque per worker)
- `engine/jobs/job.hpp` (Job handle, dependencies)

### Week 13 — Jobs submission + parallel_for (Phase 3 continues)

**Pull cards:** `P3.10`–`P3.12`.

- `engine/jobs/submit.hpp/cpp` (user-facing submit / wait API)
- `engine/jobs/parallel_for.hpp` (convenience)
- Benchmarks versus enkiTS on reference workload (Phase 3 milestone gate is week 16; this is on-track).

Phase 3's remaining three weeks (ECS archetype storage + queries + reflection + serialization primitives + *Foundations Set* milestone demo at week 16) are covered in `05_ROADMAP_AND_MILESTONES.md`. The 90-day Master Plan ends here.

---

## 4. Recommended C++ project structure (enforced)

```
cold-coffee-labs/
├── engine/
│   ├── platform/        OS abstraction — only place OS headers are permitted
│   ├── memory/          Allocators (frame, arena, pool, free-list)
│   ├── core/            Logger, asserts, Result, containers
│   │   ├── command/     ICommand + CommandStack
│   │   ├── events/      Pub/sub bus (Phase 11)
│   │   └── config/      CVars + TOML (Phase 11)
│   ├── math/            Vec/Mat/Quat/Transform
│   ├── jobs/            Fiber-based scheduler (Phase 3)
│   ├── ecs/             Archetype ECS
│   ├── render/
│   │   ├── hal/         Hardware Abstraction Layer
│   │   ├── vk/          Vulkan 1.3 backend
│   │   ├── shader/      Permutations, DXC/SPIR-V, optional Slang (Phase 17)
│   │   ├── material/    MaterialWorld, .gwmat v1 (Phase 17)
│   │   └── post/        Bloom, TAA, MB, DoF, tonemap (Phase 17)
│   ├── assets/          Import/cook/load
│   ├── scene/           Scene view + .gwscene serialization (Phase 8)
│   ├── scripting/       Dynamic-lib loader (gameplay hot-reload)
│   ├── vscript/         Visual-script IR runtime (Phase 8)
│   ├── audio/           miniaudio + Steam Audio (Phase 10)
│   ├── net/             GameNetworkingSockets + replication (Phase 14)
│   ├── physics/         Jolt (Phase 12)
│   ├── anim/            Ozz + ACL (Phase 13)
│   ├── input/           SDL3 + action maps (Phase 10)
│   ├── ui/              RmlUi (Phase 11)
│   ├── i18n/            ICU + HarfBuzz (Phase 16)
│   ├── a11y/            Accessibility (Phase 16)
│   ├── persist/         Save + SQLite (Phase 15)
│   ├── telemetry/       Sentry + custom (Phase 15)
│   ├── console/         In-game dev console (Phase 11)
│   ├── gameai/          BT + Recast/Detour (Phase 13)
│   ├── vfx/             Particles, ribbons, decals (Phase 17)
│   ├── sequencer/       Cinematics (Phase 18)
│   ├── mod/             Mod sandbox (Phase 18)
│   └── platform_services/   Steam/EOS/Galaxy (Phase 16)
├── bld/                 Rust crate — Cargo workspace (Phase 9)
├── editor/              C++ executable, statically links core + BLD (Phase 7)
├── sandbox/             C++ executable, engine feature test bed (Phase 1)
├── runtime/             C++ shipping executable (Phase 11)
├── gameplay/            Hot-reloadable C++ dylib (Phase 2)
├── tools/cook/          gw_cook content cooker (Phase 6)
├── tests/               Integration, perf, fuzz, BLD evals (Phase 1+)
├── content/             Source assets (gitignored)
├── assets/              Cooked deterministic runtime assets (tracked)
├── third_party/         CPM-managed, SHA-pinned
├── cmake/
│   ├── dependencies.cmake
│   ├── toolchains/
│   └── packaging/       CPack configs (Phase 18)
├── docs/                This directory
├── .github/workflows/
├── .clang-format
├── .clang-tidy
├── rustfmt.toml
├── .editorconfig
├── CMakeLists.txt
├── CMakePresets.json
├── CLAUDE.md
├── README.md
└── LICENSE
```

---

## 5. Dependency graph & initialization order

```
           +------------+
           |  Platform  |
           +-----+------+
                 |
                 v
           +------------+     +------------+
           |   Memory   |<----|   Core     |
           +-----+------+     +-----+------+
                 |                  |
                 +------+-----------+
                        |
                        v
                   +---------+
                   |  Math   |
                   +----+----+
                        |
                        v
                   +---------+
                   |  Jobs   |
                   +----+----+
                        |
          +-------------+--------------+
          |             |              |
          v             v              v
      +------+     +---------+    +--------+
      | ECS  |     |  HAL    |    |  FS/IO |
      +--+---+     +----+----+    +---+----+
         |              |             |
         +-----+--------+-------------+
               |
               v
        +---------------+
        | Scene / Asset |
        +-------+-------+
                |
                v
           +---------+
           | Editor  |
           +----+----+
                |
                v
           +--------+
           |  BLD   |  (linked via C-ABI)
           +--------+
```

Initialization at sandbox startup (canonical order):

1. `platform::init()` — window, input, timing, fs.
2. `memory::init()` — allocator pools and frame allocator.
3. `core::init()` — logger, assertions, config.
4. `jobs::init()` — scheduler and worker threads.
5. `render::hal::init()` — Vulkan device, queues, swapchain.
6. `ecs::init()` — World construction, system registration.
7. `assets::init()` — asset database.
8. `scene::init()` — scene view and serialization.
9. `scripting::init()` — hot-reload file watcher and gameplay_module load.
10. *(editor only)* `bld_bridge::init()` — Rust crate handle, MCP server.

Teardown reverses the order.

---

## 6. First-week code generation (ready to compile)

The following files are authoritative for week-one scaffolding. Copy verbatim into the repo; CI should be green after commit.

### 6.1 `CMakeLists.txt` (root)

```cmake
cmake_minimum_required(VERSION 3.28)

project(greywater
    VERSION 0.0.1
    DESCRIPTION "Greywater_Engine by Cold Coffee Labs"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/dependencies.cmake)

find_package(Corrosion REQUIRED)

# Greywater_Engine core (C++23 static library)
add_subdirectory(engine)

# BLD — Rust workspace compiled via Corrosion
corrosion_import_crate(
    MANIFEST_PATH bld/Cargo.toml
    PROFILE release
)

# Executables
add_subdirectory(sandbox)
add_subdirectory(editor)
add_subdirectory(runtime)
add_subdirectory(gameplay)

# Tools
add_subdirectory(tools/cook)

# Tests
enable_testing()
add_subdirectory(tests)
```

### 6.2 `CMakePresets.json`

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "dev-linux",
            "displayName": "Developer Linux (Debug)",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_C_COMPILER": "clang",
                "CMAKE_CXX_COMPILER": "clang++",
                "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/cmake/toolchains/clang.cmake"
            }
        },
        {
            "name": "dev-win",
            "displayName": "Developer Windows (Debug)",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/cmake/toolchains/clang-cl.cmake"
            }
        },
        {
            "name": "release-linux",
            "inherits": "dev-linux",
            "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
        },
        {
            "name": "release-win",
            "inherits": "dev-win",
            "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
        },
        {
            "name": "linux-asan",
            "inherits": "dev-linux",
            "cacheVariables": {
                "CMAKE_CXX_FLAGS": "-fsanitize=address -fno-omit-frame-pointer"
            }
        },
        {
            "name": "linux-ubsan",
            "inherits": "dev-linux",
            "cacheVariables": { "CMAKE_CXX_FLAGS": "-fsanitize=undefined" }
        },
        {
            "name": "linux-tsan",
            "inherits": "dev-linux",
            "cacheVariables": { "CMAKE_CXX_FLAGS": "-fsanitize=thread" }
        }
    ],
    "buildPresets": [
        { "name": "dev-linux",     "configurePreset": "dev-linux" },
        { "name": "dev-win",       "configurePreset": "dev-win" },
        { "name": "release-linux", "configurePreset": "release-linux" },
        { "name": "release-win",   "configurePreset": "release-win" },
        { "name": "linux-asan",    "configurePreset": "linux-asan" },
        { "name": "linux-ubsan",   "configurePreset": "linux-ubsan" },
        { "name": "linux-tsan",    "configurePreset": "linux-tsan" }
    ],
    "testPresets": [
        { "name": "dev-linux", "configurePreset": "dev-linux", "output": { "outputOnFailure": true } },
        { "name": "dev-win",   "configurePreset": "dev-win",   "output": { "outputOnFailure": true } }
    ]
}
```

### 6.3 `cmake/dependencies.cmake`

```cmake
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.4
)

CPMAddPackage(
    NAME volk
    GITHUB_REPOSITORY zeux/volk
    GIT_TAG 1.3.270
)

CPMAddPackage(
    NAME VulkanMemoryAllocator
    GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.1.0
)

CPMAddPackage(
    NAME doctest
    GITHUB_REPOSITORY doctest/doctest
    GIT_TAG v2.4.11
)

# Phase 7+: imgui (docking branch), imnodes, imguizmo, imgui_md
# Phase 10+: SDL3, miniaudio, Steam Audio
# Phase 11+: RmlUi, HarfBuzz, FreeType
# Phase 12+: Jolt Physics
# Phase 13+: Ozz-animation, ACL, Recast/Detour
# Phase 14+: GameNetworkingSockets, Opus
# Phase 15+: SQLite, sentry-native
# Phase 16+: ICU
```

### 6.4 `rust-toolchain.toml`

```toml
[toolchain]
channel = "stable"
components = ["rustfmt", "clippy", "rust-src"]
profile = "minimal"
```

### 6.5 `bld/Cargo.toml` (workspace)

```toml
[workspace]
resolver = "2"
members = [
    "bld-mcp",
    "bld-tools",
    "bld-provider",
    "bld-rag",
    "bld-agent",
    "bld-governance",
    "bld-bridge",
    "bld-ffi",
]

[workspace.package]
edition = "2021"
rust-version = "1.80"
version = "0.0.1"
authors = ["Cold Coffee Labs"]
license-file = "../LICENSE"

[workspace.dependencies]
serde = { version = "1", features = ["derive"] }
serde_json = "1"
tokio = { version = "1", features = ["rt-multi-thread", "macros", "sync"] }
thiserror = "2"
anyhow = "1"

[profile.release]
lto = "thin"
codegen-units = 1
```

### 6.6 `.clang-format`

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
ReferenceAlignment: Left
NamespaceIndentation: None
AllowShortFunctionsOnASingleLine: Empty
BinPackArguments: false
BinPackParameters: false
AlwaysBreakTemplateDeclarations: Yes
IncludeBlocks: Regroup
```

### 6.7 `.clang-tidy`

```yaml
Checks: >
  bugprone-*,
  cert-*,
  clang-analyzer-*,
  concurrency-*,
  cppcoreguidelines-*,
  misc-*,
  modernize-*,
  performance-*,
  portability-*,
  readability-*,
  -readability-magic-numbers,
  -modernize-use-trailing-return-type,
  -cppcoreguidelines-avoid-magic-numbers
HeaderFilterRegex: '^(?!.*third_party).*$'
WarningsAsErrors: '*'
```

### 6.8 `rustfmt.toml`

```toml
edition = "2021"
max_width = 100
use_small_heuristics = "Max"
imports_granularity = "Module"
```

### 6.9 `.editorconfig`

```ini
root = true

[*]
indent_style = space
indent_size = 4
end_of_line = lf
charset = utf-8
trim_trailing_whitespace = true
insert_final_newline = true

[*.{yml,yaml,toml,json}]
indent_size = 2

[Makefile]
indent_style = tab
```

---

## 7. Research deep-dive: bindless descriptors + BDA

Phase 4's correctness depends on getting bindless and buffer device address right (Phase 4 begins at week 17, after this master plan's 13-week horizon — but the groundwork is worth locking in early):

- Create one large descriptor set (e.g., 65 536 textures, 65 536 storage buffers) at device init.
- Use `VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT` + `VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT`.
- Texture handles are plain `u32`s in push constants.
- For per-draw buffer data, pass a `VkDeviceAddress` (u64) in push constants; shaders dereference via `GL_EXT_buffer_reference`.
- Validate with `VK_EXT_descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE` + `shaderSampledImageArrayNonUniformIndexing = VK_TRUE`.

Full treatment in `03_VULKAN_RESEARCH_GUIDE.md` §3 (Stages 2–3).

---

## 8. Daily workflow protocol (mantra)

One card. Ninety minutes. Commit. Update log. Rest.

The protocol only works if you follow it literally. See `06_KANBAN_BOARD.md` for the full set. The abbreviated form:

1. **Morning:** read today's daily log; check the Kanban; pull one card.
2. **Block 1 (90 min):** focused work on that card. No Slack, no browser, no "quick search."
3. **Lunch.**
4. **Block 2 (90 min, optional):** continue the card or pick up a small secondary. Max In Progress stays at 2.
5. **End of day:** move the card; update the log; commit.

If the day felt scattered, **stop earlier**, note the scatter in the log, and come back tomorrow. Pushing tired code is a future cost.

---

## 9. Final verification checklist (for sign-off)

Before declaring the Pre-Development Master Plan complete and entering Phase 1:

- [ ] `00_SPECIFICATION_Claude_Opus_4.7.md` reviewed and signed by Principal Architect.
- [ ] `01_ENGINE_PHILOSOPHY.md` reviewed; aesthetic palette finalized.
- [ ] `02_SYSTEMS_AND_SIMULATIONS.md` tier assignments reviewed; all Tier A items confirmed shippable in v1.
- [ ] `05_ROADMAP_AND_MILESTONES.md` week-by-week schedule accepted.
- [ ] `06_KANBAN_BOARD.md` WIP limits agreed.
- [ ] `architecture/grey-water-engine-blueprint.md` signed by stakeholders.
- [ ] `CLAUDE.md` present at repo root; referenced by `.github/workflows/ci.yml` as a mandatory read-on-start file.
- [ ] `bootstrap_command.txt` executes cleanly and produces the expected tree.
- [ ] `07_DAILY_TODO_GENERATOR.py` runs and produces the expected daily-log set.
- [ ] Founder/Principal Architect has taken one full day off immediately before Day 1 of Phase 1.

---

*Pre-developed deliberately. Week 1 has been rehearsed on paper so it ships on the first attempt.*
