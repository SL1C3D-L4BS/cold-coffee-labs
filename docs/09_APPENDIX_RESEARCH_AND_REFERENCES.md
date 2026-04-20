# 09_APPENDIX — Research & Internal References

**Status:** Reference · Living document (updated as research lands)

This appendix is the internal bibliography of the Greywater project. It catalogues the technical standards we target, the libraries we use, and the internal decisions that shape our architecture. It is **not** a survey of external products or competitors — that kind of analysis, when we do it, lives as an ADR under `docs/adr/`, summarised into a single decision and then archived.

If you arrive at this document expecting name-drops of every technique, paper, or studio that influenced modern game engines, stop. Greywater's docs refer only to: Cold Coffee Labs, Greywater_Engine, Greywater (the game), and the open standards / tooling we use by name. That discipline is intentional.

---

## 1. Standards we target

These are the open specifications and standards that Greywater conforms to. When a claim in our docs depends on a standard, the relevant section goes here.

- **C++23** — ISO/IEC 14882:2024. Full standard library where applicable; restricted in engine hot paths (see `04_LANGUAGE_RESEARCH_GUIDE.md`).
- **Vulkan 1.2 (baseline) with opportunistic 1.3 features.** See `03_VULKAN_RESEARCH_GUIDE.md` for the full capability table.
- **Rust 2024 edition** (stable; 2021 until 2024 lands on stable). BLD crate only.
- **Model Context Protocol, version 2025-11-25.** The wire protocol between BLD and any MCP client.
- **Unicode 16.0** — text handling via ICU.
- **WCAG 2.2 Level AA** — accessibility target for HUD and menus.
- **GDPR, CCPA, COPPA** — privacy-law compliance for telemetry and save data.
- **glTF 2.0** — mesh/scene interchange format.
- **KTX2** — GPU-ready texture container.
- **SPIR-V** — shader bytecode; compiled from HLSL via DXC.
- **SemVer 2.0.0** — engine versioning.
- **Conventional Commits 1.0.0** — commit-message format.

## 2. Libraries and tooling we use

A flat list of the open-source and freely-licensed libraries and tools Greywater depends on. This list is the single source of truth for "what we link against" — ADRs reference it, the CPM dependency file pins versions that match it.

### Build and toolchain
- **Clang / LLVM** — C++ compiler.
- **CMake 3.28+** with Presets — build generator.
- **Ninja** — build driver.
- **Corrosion** — CMake ↔ Cargo integration for BLD.
- **Cargo** — Rust build driver.
- **CPM.cmake** — C++ dependency manager.
- **cxx** + **cbindgen** — Rust/C++ binding generation.
- **sccache** — shared compile cache.
- **rustfmt**, **clippy**, **cargo-deny**, **cargo-audit**, **cargo-mutants**, **Miri** — Rust quality tooling.
- **clang-format**, **clang-tidy**, **cmake-format**, **editorconfig** — C++ / infrastructure quality tooling.

### Rendering
- **volk** — Vulkan loader.
- **VulkanMemoryAllocator (VMA)** — GPU memory management.
- **DXC** — HLSL → SPIR-V shader compiler.
- **spirv-cross** / **spirv-reflect** — shader reflection.
- **GLFW** — windowing backend behind `engine/platform/`.
- **Dear ImGui** (docking + viewports branch) — editor UI.
- **ImNodes** — node-graph widget for visual scripting projection.
- **ImGuizmo** — 3D transform gizmos.
- **`imgui_md`** (MD4C-based) — markdown rendering for BLD chat panel.
- **RmlUi** — in-game UI layout engine.
- **HarfBuzz** + **FreeType** — text shaping and rasterisation.

### Simulation & content
- **Jolt Physics** — rigid-body dynamics, character controller, constraints, deterministic mode.
- **Ozz-animation** + **ACL** — skeletal animation runtime + compression.
- **Recast/Detour** — navigation mesh generation and pathfinding.
- **cgltf** / **fastgltf** — glTF 2.0 import.
- **libktx** + BC7/ASTC encoders — texture cook.

### Audio
- **miniaudio** — device/mixer.
- **Steam Audio** — spatial audio (HRTF, occlusion, reflection).
- **Opus** — voice and streaming codec.
- **libopus** / **libvorbis** — codec implementations.

### Networking
- **GameNetworkingSockets** — transport layer.

### Data and persistence
- **SQLite** — local database.
- **sqlite-vss** — vector search extension for BLD RAG.
- **ICU** — Unicode runtime services.

### BLD (Rust)
- **rmcp** — Rust MCP SDK (server side).
- **tree-sitter** + language grammars — AST-aware code chunking.
- **petgraph** — graph algorithms for symbol-graph ranking.
- **rusqlite** — SQLite bindings.
- **candle** — pure-Rust ML runtime (local inference + embeddings).
- **llama-cpp-rs** — alternate local-inference runtime.
- **reqwest** + **eventsource-stream** — HTTP + SSE for cloud LLM providers.
- **serde** + **serde_json** — serialization.
- **tokio** — async runtime (single, shared across the BLD crate).
- **thiserror**, **anyhow** — error types.
- **keyring** — OS keychain access for API keys.
- **notify** — filesystem watcher for RAG incremental indexing.

### Testing and observability
- **doctest** — C++ unit testing.
- **Tracy** — frame-level CPU/GPU profiling.
- **Sentry Native SDK** — crash reporting.
- **libFuzzer** — coverage-guided fuzzing.

### Packaging
- **CPack** — installer generation.
- **MSIX** (Windows) / **AppImage** / **DEB** / **RPM** (Linux) — distribution formats.

### Platform services
- **Steamworks SDK** — Steam integration (first platform backend).
- **Epic Online Services SDK** — cross-platform services (second platform backend).

### Models
- **nomic-embed-code** — default code-aware embedding model (shipped via `candle`).

## 3. Internal reference catalogue

Greywater's own canonical documents — the ones to read rather than to summarise.

| Document | Scope |
| --- | --- |
| `CLAUDE.md` | Cold-start primer and non-negotiables |
| `docs/00_SPECIFICATION_Claude_Opus_4.7.md` | Binding constitutional constraints |
| `docs/01_ENGINE_PHILOSOPHY.md` | The Brew Doctrine |
| `docs/02_SYSTEMS_AND_SIMULATIONS.md` | Complete subsystem inventory with tiers |
| `docs/03_VULKAN_RESEARCH_GUIDE.md` | Rendering stack and the RX 580 capability table |
| `docs/04_LANGUAGE_RESEARCH_GUIDE.md` | C++23 + Rust idiom guide |
| `docs/05_ROADMAP_AND_MILESTONES.md` | 24-phase build plan + LTS phase |
| `docs/06_KANBAN_BOARD.md` | Workflow and rituals |
| `docs/08_GLOSSARY.md` | Terminology |
| `docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md` | First-90-day binding plan |
| `docs/11_HANDOFF_TO_CLAUDE_CODE.md` | Per-session protocol |
| `docs/12_ENGINEERING_PRINCIPLES.md` | Code-review playbook |
| `docs/architecture/grey-water-engine-blueprint.md` | Formal stakeholder blueprint |
| `docs/architecture/greywater-engine-core-architecture.md` | Narrative core architecture |
| `docs/games/greywater/*` | Greywater-the-game vision (README + numbered `12`–`20`) |
| `docs/adr/*` | Every architectural decision ever made |

## 4. Additions policy

New entries in this appendix follow three rules:

1. **Name only what we depend on, cite only open standards.** External products, competitor engines, specific studios, specific engineers, specific papers, and specific talks do not belong here. If an idea from outside influenced us, it has been absorbed into an ADR and then into the code. The code is the reference.
2. **Pinned versions live in `cmake/dependencies.cmake` and `bld/Cargo.lock`**, not here. This appendix names the library; the pinned version is machine-enforced.
3. **Live documents are revisited quarterly.** The library ecosystem moves. The standards move. This appendix is not a snapshot.

---

*Everything we depend on is listed. Everything we don't depend on is irrelevant to Greywater.*
