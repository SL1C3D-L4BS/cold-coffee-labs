# CODING_STANDARDS — Greywater

**Status:** Operational
**Derived from:** `00_SPECIFICATION_Claude_Opus_4.7.md` §3 + `04_LANGUAGE_RESEARCH_GUIDE.md` + `12_ENGINEERING_PRINCIPLES.md`
**Binding:** at Review. `12_ENGINEERING_PRINCIPLES.md §J` is the exhaustive checklist; this document is the crib sheet.

Every rule here has rationale in `00`, `04`, or `12`. This file is the "one page you actually keep open while coding." When in doubt, follow `12` — it is authoritative.

---

## 1. Languages

| Domain | Language | Edition/Standard | Compiler |
| --- | --- | --- | --- |
| Engine, editor, sandbox, runtime, gameplay, tools, tests | **C++23** | `CMAKE_CXX_STANDARD 23` | Clang/LLVM 18+ (clang-cl on Windows) |
| BLD (`bld/*`) | **Rust 2021→2024** | stable | rustc 1.80+ |
| Shaders | **HLSL** (primary), GLSL (ports) | — | DXC → SPIR-V (Phase 5+); glslangValidator (Phase 1 bootstrap) |

Rust appears nowhere outside `bld/`. MSVC and GCC are not supported. (`00` §2.2, `CLAUDE.md` non-negotiables #3–4.)

---

## 2. Naming

### C++
- **Types:** PascalCase — `VulkanDevice`, `World`, `EntityHandle`.
- **Functions / methods:** `snake_case` — `create_swapchain`, `register_component`.
- **Member variables:** trailing underscore — `device_`, `frame_index_`.
- **Namespaces:** `snake_case` — `gw::render`, `gw::ecs`.
- **Macros:** `UPPER_SNAKE` with `GW_` prefix — `GW_ASSERT`, `GW_LOG`, `GW_UNREACHABLE`.
- **Enums:** `enum class` always. Values PascalCase.
- **Files:** `.hpp` headers (use `#pragma once`), `.cpp` sources.

### Rust
- Standard Rust: `snake_case` everywhere, `PascalCase` types, `SCREAMING_SNAKE_CASE` constants.
- `rustfmt` defaults; `clippy` with `-D warnings`.

---

## 3. Memory & ownership (C++)

- **No raw `new` / `delete`** in `engine/`, `editor/`, `sandbox/`, `runtime/`, `gameplay/`. Exception: `main()` in sandbox/editor/runtime may own a top-level `Application` with direct construction/destruction. (`12` §A1.)
- Default to `std::unique_ptr<T>`. Use `std::shared_ptr<T>` / `gw::Ref<T>` only for **cross-thread** shared ownership. (`12` §A4, §C1.)
- **Containers own their contents** — `std::vector<std::unique_ptr<Layer>>`, not `std::vector<Layer*>`. (`12` §A3.)
- **Pass shared ownership wisely** — `const std::shared_ptr<T>&` when not taking ownership. (`12` §A5.)
- **Range-iterate ref containers by `const auto&`**. (`12` §A6.)
- **Move-assignment releases existing resource before taking the new one.** (`12` §A7.)
- **All world-space positions are `float64`.** `float32` only for local-to-chunk math and GPU vertex attributes after subtracting the floating origin. (`12` §K1.)

---

## 4. Casts, strings, aliasing

- **No C-style casts.** `static_cast` / `reinterpret_cast` / `const_cast` / `dynamic_cast`. (`12` §B1.)
- `reinterpret_cast` on byte buffers gets a `// SAFETY:` comment or uses `std::start_lifetime_as<T>` (C++23). (`12` §B3.)
- **`std::string_view` never passes to a C API.** Use `const std::string&`. (`12` §E1.)
- **Parse once at load time; never per-frame.** (`12` §E5.)
- **Windows wide-string conversion** uses `MultiByteToWideChar(CP_UTF8, ...)`. Never single-char `wchar_t` casts. (`12` §E3.)

---

## 5. Threading & jobs

- **No `std::async`, no raw `std::thread`** in engine code. All work goes through `engine/jobs/`. (`12` §I1.)
- Cross-thread lambdas capture **strong refs**, not `this`. (`12` §C3.)
- Methods intended for the render thread use the `rt_` prefix. The non-prefixed outer method submits. (`12` §C3.)

---

## 6. Lambda captures

- **No `[=]`, no `[&]`** in engine code. Capture each variable explicitly. (`12` §D1.)
- Prefer capturing individual members over `this` when only one is needed. (`12` §D2.)
- Cross-thread captures use `instance = gw::ref(this)`. (`12` §C3.)

---

## 7. Events / callbacks

- **No heap allocation for transient event objects.** Stack + pass by reference. (`12` §F1.)
- Engine work queues use `std::move_only_function<void()>` (C++23). Never `std::function`. (`12` §F3.)

---

## 8. Platform headers, containers

- **OS headers** only inside `engine/platform/`. Enforced by a custom `clang-tidy` check. (`00` §2.4, `CLAUDE.md` #11.)
- **No STL containers in engine hot paths** — use `engine/core/` containers (`gw::core::Array`, `gw::core::HashMap`, `gw::core::String`). STL is allowed in editor, tools, tests. (`00` §2.2, `CLAUDE.md` #9.)

---

## 9. Rust (BLD only)

- No `unwrap` / `expect` in `bld-ffi` or `bld-governance`. (`00` §3.)
- `#[bld_tool]` proc-macro for every MCP tool. No hand-written schemas. (`00` §3.)
- `unsafe` blocks carry `// SAFETY:` comments. Miri runs nightly on `bld-ffi`. (`00` §3, `12` §J Rust.)
- FFI types are `#[repr(C)]`. No generics, trait objects, or Rust-native errors cross the boundary. (`CLAUDE.md` #15.)

---

## 10. Greywater-game rules (additive — `engine/world/` + any game subsystem)

- **K1:** World-space positions are `f64`.
- **K2:** Cached world positions update on `world::origin_shift_event`.
- **K3:** Procedural generation is deterministic — `(seed, coord)` in, content out. No clocks, no `random_device`.
- **K4:** Chunk generation respects the 10 ms per-step budget; time-slice.
- **K5:** Rollback-critical code is a pure function of game state. No side effects.
- **K6:** Structural-integrity recomputation completes in the same frame.
- **K7:** No hardcoded planet/faction/resource/scenario names in `engine/world/`.

Full rationale: `12` §K.

---

## 11. Editor UI (additive — `editor/`, Phase 7+)

- 4-px grid spacing; typography on the defined scale (10/11/12/13/14/16/20/24 px).
- Palette is 6 tokens (Obsidian, Bean, Greywater, Crema, Brew, Signal) + 4 semantic (success, warn, error, info).
- No gradients, no drop shadows, no animated chrome.
- Keyboard-first. Every major action indexed by Ctrl+K command palette.
- UI render ≤ 2 ms at 1080p on RX 580.
- Every selectable element's right-click menu includes "Ask BLD about this."

Full specification: `architecture/greywater-engine-core-architecture.md` Chapter 13, `architecture/grey-water-engine-blueprint.md §3.31`, `01 §3.6`.

---

## 12. Tooling enforcement

| Tool | Config | Enforced by |
| --- | --- | --- |
| `clang-format` | `.clang-format` | CI `lint` job + pre-commit |
| `clang-tidy` | `.clang-tidy` | CI (Phase 2+ once it's wired into the per-TU build) |
| `rustfmt` | `rustfmt.toml` | `cargo fmt --check` in CI |
| `clippy` | — | `cargo clippy -- -D warnings` in CI |

Non-conforming PRs are blocked on CI. See `12` §J for the exhaustive review rubric.

---

*Standardized deliberately. Every rule here is a scar someone already paid for. Honor the scar.*
