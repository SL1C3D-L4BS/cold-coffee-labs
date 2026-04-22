# 05 — Research, Build & Coding Standards

**Status:** Reference · Operational  
**Precedence:** L6 — build detail and language discipline; specific decisions superseded by ADRs

---

---


---

## Vulkan 1.2-Baseline Research Guide

---

## 1. Why Vulkan 1.2-baseline (and not higher)

**The RX 580 is our baseline.** AMD's Polaris architecture (2016) supports **Vulkan 1.2 fully** and most of Vulkan 1.3 via driver extensions. We target **Vulkan 1.2 as the floor** and enable Vulkan 1.3 features **opportunistically** via `gw::render::RHICapabilities` when the driver reports support.

### 1.1 Features we use on RX 580 (Tier A — ships)

- **Dynamic rendering** (`VK_KHR_dynamic_rendering`) — baseline on current RX 580 drivers.
- **Timeline semaphores** (`VK_KHR_timeline_semaphore`, core 1.2) — baseline.
- **Synchronization 2** (`VK_KHR_synchronization2`) — baseline with current AMD drivers.
- **Bindless descriptors** (`VK_EXT_descriptor_indexing`, core 1.2) — baseline.
- **Buffer device address** (`VK_KHR_buffer_device_address`, core 1.2) — baseline.
- **Compute shaders** — GCN 4.0 is fully compute-capable.
- **Async compute queue** — AMD has native compute queues; we use them.
- **Hardware tessellation** — baseline.
- **Descriptor update after bind** (`VK_EXT_descriptor_indexing`) — baseline.

### 1.2 Features we do NOT use on RX 580 (Tier G — hardware-gated, post-v1)

- **Ray tracing pipeline** (`VK_KHR_ray_tracing_pipeline`) — RX 580 has no RT cores.
- **Acceleration structures** (`VK_KHR_acceleration_structure`) — same.
- **Ray query** (`VK_KHR_ray_query`) — same.
- **Mesh shaders** (`VK_EXT_mesh_shader`) — Polaris doesn't have them. Emulation exists but is not a production path.
- **GPU work graphs** (Vulkan 1.4 extension) — post-Polaris hardware only.

On hardware that reports support for these features, we light them up behind `RHICapabilities` flags. The baseline renderer path never depends on them.

### 1.3 What this costs us

- **No real-time ray-traced reflections / shadows / GI.** We substitute screen-space techniques (SSAO, SSR) and offline-baked IBL.
- **No mesh-shader-driven variable-rate culling.** We use compute-shader-driven GPU culling with `vkCmdDrawIndexedIndirect`. In practice this is 95 % of the benefit for our geometry budgets.
- **No work-graph-driven GPU pipelining.** The CPU orchestrates the frame graph. This is how every engine worked until 2024; it remains the correct architecture on our hardware.

### 1.4 Target performance

- **1080p @ 60 FPS** on RX 580 reference hardware for the full simulation:
  - Sponza-class terrain geometry.
  - 2 km visible view distance.
  - 10 000 active ECS entities.
  - Full atmospheric scattering + volumetric clouds.
  - Cascaded shadow maps.
  - PBR materials + IBL.
- Perf regression ≥ 5 % on the canonical scene **fails CI**.

---

## 2. Greywater's HAL commitments

The Hardware Abstraction Layer (`engine/render/hal/`) is a ~30-type C++ API. It is **not** a Vulkan wrapper — it is GPU vocabulary expressed in a backend-agnostic form, with Vulkan as the first backend.

**What the HAL exposes:**
- `Device`, `Queue`, `CommandBuffer`, `Swapchain`
- `Buffer`, `Image`, `Sampler`, `ImageView`
- `Pipeline`, `PipelineLayout`, `DescriptorSet` (bindless-friendly)
- `Fence`, `TimelineSemaphore`
- `BarrierDesc`, `ImageLayout`, `AccessFlags`, `StageFlags`
- `RenderingInfo` (dynamic-rendering scope descriptor)
- `RHICapabilities` (feature flags; see §4)
- `FrameGraph` (above the HAL; not part of the abstraction proper)

**What the HAL does not expose:**
- Individual Vulkan function pointers. Go through the backend.
- Raw `VkDevice` / `VkImage` / `VkBuffer`. Never exposed past the backend boundary.
- Render-pass objects. We use dynamic rendering.
- Descriptor pools. Bindless amortizes.

**Backend:** `engine/render/vk/` is the Vulkan 1.2-baseline implementation. `engine/render/webgpu/` is planned post-v1.

---

## 3. Staged learning path (for someone new to Vulkan)

If you've never written Vulkan before, work through these in order before touching Greywater's renderer.

### Stage 1 — Modern Vulkan fundamentals (est. 40 h)
- vulkan-tutorial.com's *"Dynamic rendering"* branch (not the main branch — the main branch teaches pre-1.3 patterns that are obsolete).
- Sascha Willems' `dynamicrendering` sample.
- The official Vulkan `synchronization-2` sample.

**Commit 1.3 idioms from day one.** If you find yourself typing `VkRenderPassCreateInfo`, stop.

### Stage 2 — Buffer device address and push constants (est. 20 h)
- Nabla's BDA-centric sample.
- The Vulkan Guide (`vkguide.dev`)'s recent chapters covering BDA.

Pattern: uniforms and per-draw data are passed as 64-bit device-address pointers in push constants; shaders dereference them via `GL_EXT_buffer_reference`. This eliminates 90 % of descriptor-set boilerplate.

### Stage 3 — Bindless (est. 30 h)
- `VK_EXT_descriptor_indexing` (core 1.2) + `VK_EXT_mutable_descriptor_type`.
- One large descriptor set per type class (textures, samplers, buffers) with indexed access from shaders.
- Greywater's pattern: `TextureHandle = u32`, shaders index `textures[TextureHandle]`.

### Stage 4 — Frame graphs (est. 40 h)
- Any modern frame-graph reference that treats resource lifetimes and barrier insertion as first-class concerns. The design pattern is well-documented in the graphics literature.
- Granite's frame graph implementation (open source, excellent reference).
- Implement a toy frame graph for a forward+ pipeline with depth pre-pass, culling, opaque, transparent, post.

Greywater's frame graph lives above the HAL. Render passes declare their reads/writes; the graph inserts barriers, aliases transient resources, and schedules on the job system.

### Stage 5 — Multi-queue and async compute on AMD (est. 30 h)
- Use timeline semaphores to coordinate graphics and compute queues.
- Migrate culling, light clustering, and particle simulation to async compute.
- AMD's async compute queue is fast on Polaris — lean on it.

### Stage 6 — Profiling and debugging (ongoing)
- **RenderDoc** for frame captures. Always debug with validation layers on.
- **Radeon GPU Profiler** (AMD) for hardware timings — the right tool on our target GPU.
- **Tracy** GPU zones for CPU/GPU correlation.

---

## 4. Capability gates (`RHICapabilities`)

Greywater runs on a range of hardware. Features land behind a single `RHICapabilities` struct populated at device init. Code paths conditionally take advantage.

```cpp
namespace gw::render {

struct RHICapabilities {
    // Baseline (required — absence is a fatal error for the Tier A renderer)
    bool dynamic_rendering;              // VK_KHR_dynamic_rendering (baseline on RX 580)
    bool timeline_semaphores;            // core 1.2
    bool synchronization_2;              // VK_KHR_synchronization2 (baseline on RX 580)
    bool bindless_descriptors;           // VK_EXT_descriptor_indexing (core 1.2)
    bool buffer_device_address;          // core 1.2
    bool compute_shaders;                // baseline on Polaris and newer
    bool async_compute_queue;            // AMD native; most modern NVIDIA too

    // Opportunistic (light up when present; fall back gracefully when absent)
    bool mesh_shaders;                   // VK_EXT_mesh_shader — false on RX 580
    bool gpu_work_graphs;                // Vulkan 1.4 — false on RX 580
    bool ray_tracing_pipeline;           // VK_KHR_ray_tracing_pipeline — false on RX 580
    bool ray_query;                      // VK_KHR_ray_query — false on RX 580
    bool shader_int64;                   // for some bindless patterns
    bool cooperative_matrix;             // for future ML-in-shader workloads
    u32  max_descriptor_array_size;      // bindless ceiling
    u64  total_vram_bytes;               // on RX 580: 8 * 1024 * 1024 * 1024
};

} // namespace gw::render
```

Callers gate on the booleans. No runtime `if (mesh_shaders) ...` strewn through shading code — the **frame graph** swaps its culling pass depending on capability.

---

## 5. Shader toolchain

- **Authoring:** HLSL primary (Microsoft's shading language, now open, with excellent tooling and SPIR-V emission through DXC). GLSL supported for ports and for legacy samples.
- **Compiler:** **DXC** (DirectX Shader Compiler) emits SPIR-V. Invoked from `tools/cook/` at content-cook time and from the editor for hot-reload.
- **Permutations:** generated at cook time from feature-flag matrices declared per material. No runtime shader compilation in shipped builds.
- **Include paths:** `shaders/common/` for shared, `shaders/<subsystem>/` for specific.
- **Reflection:** `spirv-cross` or `spirv-reflect` for shader introspection at cook time.
- **Shader hot-reload:** the editor watches `shaders/` and recompiles on change; GPU pipeline cache is keyed by content hash.
- **Low-poly shading:** our default shading style is flat or subtle-gradient. Shader permutations include a `FLAT_SHADING` flag that computes normals from screen-space derivatives (`dFdx`/`dFdy`) for a faceted look without pre-baking flat normals into geometry.

---

## 6. Frame graph pattern (Greywater reference)

A frame is a directed acyclic graph of passes. Each pass declares reads/writes on resources; the graph compiler:

1. Topologically sorts passes.
2. Aliases transient resources (same lifetime → same GPU memory).
3. Inserts barriers with correct stage/access masks.
4. Schedules independent passes onto async compute queue where beneficial.
5. Produces a linear command-buffer recording list.

Pseudocode (Greywater shape):

```cpp
FrameGraph fg;

auto depth         = fg.create_texture("depth", {depth24, 1920, 1080});
auto gbuffer_albedo = fg.create_texture("gbuffer_albedo", {rgba8, 1920, 1080});
auto light_grid    = fg.create_buffer("light_grid", {/* cluster data */});
auto terrain_mesh  = fg.create_buffer("terrain_mesh", {/* compute-generated */});

fg.add_pass("terrain_gen", QueueType::Compute)
  .writes(terrain_mesh, WriteMode::ShaderWrite)
  .reads(chunk_params, ReadMode::Uniform)
  .execute([&](CommandBuffer& cb) {
      // dispatch compute shader to generate a terrain chunk
  });

fg.add_pass("depth_prepass")
  .reads(terrain_mesh, ReadMode::VertexAttribute)
  .writes(depth, WriteMode::DepthStencil)
  .execute([&](CommandBuffer& cb) {
      // record depth-only draws
  });

fg.add_pass("light_clustering", QueueType::Compute)
  .reads(depth, ReadMode::ShaderSample)
  .writes(light_grid, WriteMode::ShaderWrite)
  .execute([&](CommandBuffer& cb) {
      // dispatch compute
  });

fg.add_pass("forward_opaque")
  .reads(depth, ReadMode::DepthRead)
  .reads(light_grid, ReadMode::ShaderSample)
  .reads(terrain_mesh, ReadMode::VertexAttribute)
  .writes(gbuffer_albedo, WriteMode::ColorAttachment)
  .execute([&](CommandBuffer& cb) {
      // record forward draws
  });

fg.compile();     // validate, alias, insert barriers
fg.execute();     // record command buffers, submit
```

---

## 7. Performance targets and benchmarks

Exit criteria for Phase 5 (*Sponza Lit*) are measured on **RX 580 reference hardware**:

| Scene                       | Target                                              |
| --------------------------- | --------------------------------------------------- |
| Sponza (baked materials)    | ≥ 60 FPS @ 1080p, forward+ with CSM and IBL         |
| Greywater hero scene        | ≥ 60 FPS @ 1080p, atmosphere + clouds + 2 km range  |
| Dense particle sandbox      | ≥ 60 FPS @ 1080p with 100k GPU particles            |
| Instancing stress           | ≥ 60 FPS @ 1080p with 20k instanced low-poly meshes |
| Shadow cascade correctness  | Zero shimmer under camera rotation at 60 FPS        |

On higher-end hardware (RTX 3060, RX 7800 XT class) the engine should **use the same frame graph** and hit proportionally higher frame rates at 1440p. There is no "Ultra preset" that flips different code paths — there is the Tier A renderer, plus capability-gated visual upgrades.

Benchmarks run nightly in CI on an RX-580-class reference runner and compare to previous baselines. A > 5 % regression fails the build.

---

## 8. Common pitfalls (to avoid, not rediscover)

- **Do not share descriptor sets across frames-in-flight.** Use ring-buffered bindless descriptors.
- **Do not submit with binary semaphores** if a timeline semaphore would work.
- **Do not allocate per-frame memory from the device heap.** Use a transient frame allocator backed by a persistent buffer.
- **Do not forget `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`** — introduced by `synchronization_2`, replaces several older read-only layouts, strictly better.
- **Do not use `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT`** as a lazy barrier. Validation won't catch it, but your performance will.
- **Do not rely on driver defaults** for extension feature enablement. Always enable explicitly and check `RHICapabilities`.
- **Do not budget VRAM above 7 GB.** On an 8 GB card, you need overhead for the OS compositor and for driver allocations.
- **Do not design a Tier A feature that requires mesh shaders or ray tracing.** Gate it; fall back; ship the baseline.

---

## 9. Greywater-specific Vulkan features

Beyond the general renderer patterns, Greywater uses Vulkan for game-specific workloads:

- **GPU terrain / mesh generation** — compute shaders generate meshes from deterministic fields (arenas, circles, Blacklake GPTM slices). See `docs/08_BLACKLAKE_AND_RIPPLE_STUDIO_SPECS.md` and `docs/04_SYSTEMS_INVENTORY.md`.
- **Atmospheric and volumetric effects** — scattering and fog paths that fit RX 580; *Sacrilege* pushes local volumetrics and horror reads (GW-SAC-001). See `docs/06_ARCHITECTURE.md` rendering chapters.
- **Raymarched volumetric clouds** — compute shader, 3D Perlin-Worley density texture.
- **GPU procedural vegetation** — compute shader emits `VkDrawIndexedIndirectCommand` buffers; culling + LOD selection on GPU.
- **Async compute for terrain** — chunk generation runs on the compute queue while the graphics queue renders the previous frame.

---

## 10. Required reading

- [Vulkan 1.2 Specification](https://registry.khronos.org/vulkan/specs/1.2-extensions/html/)
- [Vulkan 1.3 Specification](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/) — for opportunistic features
- Vulkan Guide — [`vkguide.dev`](https://vkguide.dev/) (recent chapters are 1.3-correct)
- Sascha Willems' Vulkan samples — [`github.com/SaschaWillems/Vulkan`](https://github.com/SaschaWillems/Vulkan)
- Official Vulkan Samples repo (synchronization-2 examples and more).
- Granite — Themaister's engine — reference for modern Vulkan rendering patterns
- The Vulkan specification itself — §3.9 (synchronization-2), §14 (descriptor indexing), §17 (dynamic rendering) are the load-bearing reads.
- AMD GPUOpen performance guide for Polaris

---

*Rendered deliberately. Vulkan 1.2 is the baseline. RX 580 is the target. Do not write features that shut out the player base.*


---


---

## Language Research Guide (C++23 + Rust)

---

## 1. The split — why two languages, and where the seam runs

- **C++23** for the engine core, editor, sandbox, runtime, gameplay module. This is 95 % of the codebase.
- **Rust (stable, 2024 edition)** for BLD — the AI copilot subsystem — and nothing else.
- The seam is a **narrow C-ABI** generated by `cbindgen` and consumed via `cxx`. Only handles and POD messages cross.

Full rationale in `docs/03_PHILOSOPHY_AND_ENGINEERING.md` and `docs/06_ARCHITECTURE.md` §3.7. This document covers the *how* — idioms, patterns, pitfalls.

---

## 2. C++23 discipline

### 2.1 The forbidden list

Disallowed in engine code, no exceptions:

- `std::async`, `std::thread` — use `engine/jobs/`.
- `new` / `delete` at the global level — allocators are passed explicitly.
- `std::vector`/`std::unordered_map`/`std::list`/`std::deque` in hot paths — use `engine/core/` containers.
- Exceptions — validated at system boundaries via `Result<T, E>`. No `throw` in library code.
- RTTI (`dynamic_cast`, `typeid`) in hot paths.
- Virtual functions in hot loops — use CRTP, `std::variant`, or dispatch tables.
- OS headers (`<windows.h>`, `<unistd.h>`, etc.) outside `engine/platform/`.
- Unqualified `using namespace`. Especially `using namespace std`.

Allowed and encouraged:

- `constexpr`, `consteval`, `if constexpr`, `if consteval`.
- Concepts for template constraints.
- `[[nodiscard]]`, `[[likely]]`, `[[unlikely]]`, `[[no_unique_address]]`.
- `std::span`, `std::optional`, `std::string_view`.
- C++23 `deducing this` where it simplifies CRTP.
- Modules (`export module gw.core.math;`) once Clang support stabilizes across CI.

### 2.2 Naming conventions (repeat from `docs/01_CONSTITUTION_AND_PROGRAM.md`)

- Types: `PascalCase`
- Functions / methods: `snake_case`
- Members: trailing underscore (`device_`)
- Namespaces: `snake_case` (`gw::render`)
- Macros: `UPPER_SNAKE` with `GW_` prefix

### 2.3 Memory idioms

**Allocator injection.** Every API that allocates accepts an allocator by reference.

```cpp
auto buffer = Buffer::create(device, allocator_, {.size = 1024});
auto scratch = FrameAllocator::current();
auto points = scratch.alloc<Vec3>(100);
```

**RAII ownership, handle-based sharing.** Resources are owned by one place; everywhere else holds a handle. For the ECS, entities are handles (`EntityHandle`); for GPU resources, `TextureHandle`, `BufferHandle`.

**No hidden allocations.** If a function allocates, it takes an allocator. If it doesn't, it's noexcept.

### 2.4 Error handling

`Result<T, E>` is the canonical error-returning type. Boundaries validate; interior functions propagate.

```cpp
Result<Image, ImageLoadError> load_image(Path path)
{
    auto bytes = io::read_bytes(path);
    if (!bytes) return bytes.error();
    auto image = decoder::decode(*bytes);
    if (!image) return image.error();
    return *image;
}
```

No exceptions. No bare error codes. Bring back `GW_TRY(expr)` for the common case:

```cpp
Result<Mesh, MeshLoadError> load_mesh(Path path)
{
    GW_TRY(auto image, load_image(path.with_extension("png")));
    GW_TRY(auto geom, load_geometry(path.with_extension("gltf")));
    return Mesh::combine(image, geom);
}
```

### 2.5 Concurrency idioms

All concurrent work goes through `engine/jobs/`. The hot pattern:

```cpp
auto handle = jobs::submit("cook_texture", [src, dst]() {
    cook::texture_to_ktx2(src, dst);
});
handle.wait();
```

For parallel-for:

```cpp
jobs::parallel_for(entities.size(), [&](size_t i) {
    update_transform(entities[i]);
});
```

Atomics are allowed. Mutexes are allowed but rare; prefer lock-free queues for inter-thread communication.

### 2.6 Compile-time patterns

- Component reflection is macro-based for Phase 2; migration to C++26 P2996 static reflection planned once Clang ships it.
- Use `consteval` for work that must happen at compile time (shader permutation tables, component layouts).
- `if constexpr` for template branches.

### 2.7 Lifetimes and ownership

C++ doesn't have Rust's borrow checker. Discipline replaces it:

- **One owner per resource.** If multiple paths can "free" it, you have a bug.
- **Handles, not pointers**, across subsystem boundaries. A handle is a plain integer; the owner is always known.
- **No raw `new T`** at call sites. Use factory functions that return `std::unique_ptr` or a handle.
- **RAII is the default.** If cleanup happens in a destructor, you do not write cleanup at every return site.

---

## 3. Rust discipline (BLD only)

Rust is used only inside `bld/`. Different discipline applies. All the engine-core rules do not apply here — `Vec`, `HashMap`, `Arc<Mutex<_>>`, and standard idioms are fine.

### 3.1 Crate layout

`bld/` is a Cargo workspace. Each sub-crate is a library (`bld-mcp`, `bld-tools`, `bld-provider`, `bld-rag`, `bld-agent`, `bld-governance`, `bld-bridge`, `bld-ffi`).

Separate crates have separate responsibilities and separate test suites. Avoid circular dependencies — the dependency graph is a DAG:

```
bld-ffi ← bld-bridge ← bld-agent ← bld-provider, bld-tools, bld-rag, bld-governance
                                     ↑                              ↓
                                     └──────── bld-mcp ────────────┘
```

### 3.2 Error handling

- Library crates use **`thiserror`** for typed errors:
  ```rust
  #[derive(thiserror::Error, Debug)]
  pub enum ProviderError {
      #[error("upstream HTTP error: {0}")]
      Http(#[from] reqwest::Error),
      #[error("model returned malformed JSON at byte {offset}")]
      MalformedJson { offset: usize },
  }
  ```
- The outermost crate (`bld-agent`) can use **`anyhow`** at the top of the agent loop for ergonomic `?`.
- `unwrap()`/`expect()` are **forbidden** in `bld-ffi` and `bld-governance`. Elsewhere: permitted in tests, discouraged in production code.

### 3.3 `unsafe` discipline

Required `// SAFETY:` comment above every `unsafe` block. A custom clippy lint enforces.

```rust
// SAFETY: `handle` is guaranteed valid for the duration of this call
// because the caller holds a reference in `bld_ffi::SessionGuard`.
let session = unsafe { &*handle.cast::<Session>() };
```

`unsafe` appears in `bld-ffi` (the C-ABI layer) and occasionally in `bld-rag` for tree-sitter FFI. Nowhere else.

### 3.4 The `#[bld_tool]` macro

All agent-callable tools are defined with the `#[bld_tool]` attribute macro, implemented in `bld-tools-macros` (proc-macro crate; see ADR-0012). The macro generates:

- The MCP tool schema (JSON Schema) via `schemars`
- The JSON-argument decoder (`#[derive(serde::Deserialize)]` on a synthetic `__<fn>_Args` struct)
- A `ToolDescriptor` entry and its `inventory::submit!` registration hook
- A unit test that the schema is valid under `jsonschema::Validator::new`

Component tools (`component.<Name>.{get,set,add,remove}`) are **autogenerated at registration time** by `bld_component_tools!()`, walking the reflection registry via the Surface P v2 `gw_editor_enumerate_components` ABI.

See `snippets/bld_tool_example.rs` for the canonical shape.

### 3.4.1 Provider abstraction (ADR-0010)

The LLM provider abstraction is our own trait, `ILLMProvider`, wrapping `rig-core` as the carrier. Every backend — Anthropic Claude cloud, `mistral.rs` primary local, `llama-cpp-2` fallback local — implements the same trait. Contract tests in `bld-provider/tests/contract.rs` run a shared 20-prompt suite against every implementation; pass-rate regression < 95 % fails CI.

Cloud secrets load via `keyring = "4"` with platform-native features (`windows-native`, `linux-native`). An opaque `ApiKey` newtype redacts through `Debug`/`Display` and zeroises on `Drop`. `tracing` field filters reject any span carrying `api_key = *`.

### 3.5 Async discipline

- **Single async runtime:** `tokio`, configured once at BLD init. No mixing runtimes.
- **Prefer `spawn` over blocking.** If a tool needs to block, spawn a blocking task (`tokio::task::spawn_blocking`).
- **Cancellation is cooperative.** Every long-running task checks a `CancellationToken` passed in.
- **No blocking I/O in async contexts.** Violations are found by `tokio-console` during development.

### 3.5.1 RAG pipeline (ADR-0013)

Two indices:
- **Engine-docs index** — prebuilt on developer boxes with Nomic Embed Code (7 B) and shipped as `assets/rag/engine_docs.vec.sqlite`.
- **User-project index** — built at runtime with `nomic-embed-text-v2-moe` via `fastembed-rs` (Candle backend, 150 MB VRAM).

Storage is `rusqlite` + **`sqlite-vec`** (sqlite-vss deprecated upstream). Chunker is `tree-sitter` with grammars for C/C++, Rust, GLSL, JSON, Markdown, and two custom grammars for `.gwvs` (vscript IR) and `.gwscene` text headers. Symbol graph is `petgraph` with PageRank for re-ranking. Watcher is `notify-debouncer-mini` with 2 s debounce + 30 s quiet window — no re-embed during typing.

### 3.5.2 Structured output (ADR-0016)

Local providers use `mistral.rs` `Model::generate_structured` with JSON-Schema constraints; no GBNF pipeline in the common path. The `llama-cpp-2` fallback compiles schemas to GBNF via `json_schema_to_grammar` → `LlamaGrammar::parse` at provider init.

### 3.6 Serialization

- `serde` + `serde_json` are the defaults.
- Public struct shapes that cross the MCP wire use `#[derive(Deserialize, Serialize)]` with explicit `#[serde(rename_all = "snake_case")]`.
- Never `to_string()` a struct when you could `to_writer()` directly — saves an allocation.

### 3.7 FFI discipline

The `bld-ffi` crate is the boundary. Rules:

- **`#[repr(C)]` on every struct that crosses.** Layout must be stable.
- **No enums with data.** C-compatible enums only; use discriminants + out-params.
- **Opaque handles, not references.** The C++ side never dereferences a Rust pointer; it calls back through typed functions.
- **All pointers are `*mut`/`*const`**, never `&`. Aliasing rules are the caller's contract.
- **Miri nightly** validates UB-freedom.

---

## 4. The FFI seam — idioms on both sides

### 4.1 Rust side (`bld-ffi`)

```rust
#[repr(C)]
pub struct BldSessionHandle {
    _opaque: [u8; 0],   // size-zero; callers cast to *mut BldSession
}

#[no_mangle]
pub extern "C" fn bld_session_create(
    config: *const BldSessionConfig,
) -> *mut BldSessionHandle {
    // SAFETY: config is non-null per the C header contract (documented).
    let config = unsafe { &*config };
    let session = Box::new(Session::new(config.clone()));
    Box::into_raw(session).cast()
}

#[no_mangle]
pub unsafe extern "C" fn bld_session_destroy(handle: *mut BldSessionHandle) {
    if handle.is_null() {
        return;
    }
    drop(Box::from_raw(handle.cast::<Session>()));
}
```

### 4.2 C++ side (`editor/bld_bridge/`)

```cpp
#include "bld_ffi.h"  // generated by cbindgen
#include <memory>

namespace gw::editor::bld {

struct SessionDeleter {
    void operator()(BldSessionHandle* h) const noexcept { bld_session_destroy(h); }
};

using Session = std::unique_ptr<BldSessionHandle, SessionDeleter>;

inline Session create_session(const BldSessionConfig& config) {
    return Session{bld_session_create(&config)};
}

} // namespace gw::editor::bld
```

Ownership is RAII on the C++ side; the Rust side creates and the `unique_ptr` destructor calls back into Rust to free.

---

## 5. Required reading (primary specifications)

### C++23
- **C++23 Standard (draft):** [eel.is/c++draft](https://eel.is/c++draft/)
- **cppreference** — authoritative library and language reference: [en.cppreference.com](https://en.cppreference.com/)

### Rust (2024 edition)
- **The Rust Standard Library Reference:** [doc.rust-lang.org/std/](https://doc.rust-lang.org/std/)
- **The Rust Reference (language):** [doc.rust-lang.org/reference/](https://doc.rust-lang.org/reference/)
- **`cxx` documentation** — [cxx.rs](https://cxx.rs/) (the generator we use for Rust/C++ bridging).
- **Unsafe-Rust guidelines** — UCG (Unsafe Code Guidelines) working group output.

### FFI
- The `cxx` generator's own safety documentation.
- Miri's documentation on what UB it detects — [rust-lang/miri](https://github.com/rust-lang/miri).

Greywater does not maintain an external curriculum of C++ or Rust books here. Engineers onboarding are expected to be proficient before arrival; internal onboarding uses `docs/01_CONSTITUTION_AND_PROGRAM.md` and the §12 code-review playbook.

---

## 6. Tooling cheat sheet

### C++
- Format: `clang-format -i <file>`
- Lint: `clang-tidy <file>`
- Build: `cmake --build --preset dev-linux`
- Test: `ctest --preset dev-linux`
- Sanitizer run: `cmake --preset linux-asan && cmake --build --preset linux-asan && ctest --preset linux-asan`

### Rust
- Format: `cargo fmt`
- Lint: `cargo clippy -- -D warnings`
- Build: `cargo build --release`
- Test: `cargo test`
- UB check: `cargo +nightly miri test -p bld-ffi`
- Dependency audit: `cargo deny check`
- Security audit: `cargo audit`

### Cross-language
- `bindgen` headers → Rust (rarely needed; most traffic goes the other direction).
- `cbindgen` Rust → C header. Regenerate after `bld-ffi` changes:
  ```
  cbindgen --config bld/cbindgen.toml --crate bld-ffi --output include/bld.h
  ```

---

*Two languages. One codebase. Both disciplined.*


---


---

## Build Instructions

**Audience:** anyone who has just cloned the repo and wants a working build.
**Status:** Operational · Phase 1 minimum. Phase 2+ adds per-subsystem detail.

---

## Prerequisites

| Requirement | Minimum | Install |
| --- | --- | --- |
| Clang / LLVM | **18+** (we use 18–22) | `apt install clang-18 lld-18` (Linux) · LLVM installer or Chocolatey `choco install llvm` (Windows) |
| CMake | **3.28+** | https://cmake.org/download/ |
| Ninja | **1.11+** | `apt install ninja-build` · https://github.com/ninja-build/ninja/releases |
| Rust (stable) | **1.80+** | https://rustup.rs/ |
| Python | **3.10+** | (for `git history`) |
| Vulkan SDK | **1.4.341.1** (CI-pinned; engine targets 1.2 core + opportunistic 1.3) | https://vulkan.lunarg.com/sdk/home |
| Git | any | |

On Windows we use **clang-cl** (LLVM's MSVC-compatible driver). MSVC and GCC are not supported (`docs/00` §2.2).

Optional: `sccache` — pass `-DGW_USE_SCCACHE=ON` at configure time to wire it as the compiler launcher.

---

## First build

```bash
# Linux
cmake --preset dev-linux
cmake --build --preset dev-linux
ctest --preset dev-linux

# Windows (developer PowerShell or Developer Command Prompt is NOT required —
# clang-cl ships independently of Visual Studio)
cmake --preset dev-win
cmake --build --preset dev-win
ctest --preset dev-win
```

The first configure downloads CPM-managed dependencies (GLFW, volk, VMA, doctest, Corrosion) into `build/<preset>/_deps/`. Subsequent configures are offline.

Run the Phase-1 *First Light* binaries:

```bash
./build/dev-linux/bin/gw_sandbox   # hello-triangle
./build/dev-linux/bin/gw_editor    # prints the BLD greeting over the C-ABI
./build/dev-linux/bin/gw_tests     # doctest smoke suite
```

On Windows: `.\build\dev-win\bin\gw_sandbox.exe`, etc.

---

## Available presets

| Preset | Config | OS | Purpose |
| --- | --- | --- | --- |
| `dev-linux` | Debug | Linux | Default developer workflow |
| `dev-win` | Debug | Windows | Default developer workflow |
| `release-linux` | Release | Linux | Release build |
| `release-win` | Release | Windows | Release build |
| `linux-asan` | Debug + ASan | Linux | Nightly AddressSanitizer |
| `linux-ubsan` | Debug + UBSan | Linux | Nightly UndefinedBehaviorSanitizer |
| `linux-tsan` | Debug + TSan | Linux | Nightly ThreadSanitizer |
| `studio-linux` | see `CMakePresets.json` | Linux | Phase 17 *Studio Renderer* matrix (Slang, SPIRV-Reflect, DXC, SM69); `phase17_*` perf + ship-ready CTests |
| `studio-win` | see `CMakePresets.json` | Windows | Same on Windows |

Workflow presets exist for `dev-linux` and `dev-win` — `cmake --workflow --preset dev-linux` does configure + build + test in one invocation. Weekly CI also runs `studio-{win,linux}` (ADR-0082).

---

## Rust side (BLD workspace)

Cargo reads `rust-toolchain.toml` and `bld/Cargo.toml` automatically. CMake drives Cargo through Corrosion as part of the main build; call Cargo directly only for tighter iteration:

```bash
cd bld
cargo build --release
cargo test
cargo clippy --workspace --all-targets --release -- -D warnings
cargo fmt --all
cargo +nightly miri test -p bld-ffi   # UB check on the FFI boundary (nightly)
```

`bld-ffi` is produced as both a `staticlib` and a `cdylib`; the editor and sandbox binaries link against the static archive.

---

## Troubleshooting

**`glslangValidator not found`** — install the Vulkan SDK or set `VULKAN_SDK` to its root directory. The sandbox compiles shaders at build time.

**`Corrosion not found` / Rust build errors** — ensure `rustup` / `cargo` are on PATH. Corrosion is fetched via CPM on first configure.

**Linker errors against Rust static library on Windows** — the editor's CMakeLists links `ws2_32`, `userenv`, `ntdll`, `bcrypt`. If you add new Rust crates that require more platform libraries, add them there.

**Validation layer warnings** — Debug builds enable `VK_LAYER_KHRONOS_validation`. Install the SDK's Vulkan-Loader component or set `VK_LAYER_PATH` to the SDK's layer directory.

**Line-ending warnings** — `.gitattributes` normalizes to LF. If you see warnings on Windows, run `git add --renormalize .` once.

---

## What's *not* here

Everything beyond the Phase 1 smoke. Run `git log` for landed work. The phase-gate milestone (`First Light`, week 004 — `docs/02_ROADMAP_AND_OPERATIONS.md`) demands a two-minute recorded demo of the sandbox triangle + editor greeting on both Windows and Linux; that is stored under `docs/milestones/` when recorded.

---

*Built deliberately. Clang-only. Ninja-only. Windows + Linux equal. Shipped by Cold Coffee Labs.*


---


---

## Coding Standards (Quick Reference)

**Status:** Operational
**Derived from:** `docs/01_CONSTITUTION_AND_PROGRAM.md` §3 + `docs/05_RESEARCH_BUILD_AND_STANDARDS.md` + `docs/03_PHILOSOPHY_AND_ENGINEERING.md`
**Binding:** at Review. `docs/03_PHILOSOPHY_AND_ENGINEERING.md §J` is the exhaustive checklist; this document is the crib sheet.

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

## 10. `engine/world/` rules (additive — Blacklake / *Sacrilege* / any game subsystem using world streaming)

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

Full specification: `docs/06_ARCHITECTURE.md` Chapter 13, `docs/06_ARCHITECTURE.md §3.31`, `01 §3.6`.

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


---


---

```text
mkdir -p engine/{platform,memory,core/{command,events,config},math,jobs,ecs,render/{hal,shader,material},assets,scene,scripting,vscript,audio,net,physics,anim,input,ui,i18n,a11y,persist,telemetry,console,gameai,vfx,sequencer,mod,platform_services} bld/{bld-mcp,bld-tools,bld-provider,bld-rag,bld-agent,bld-governance,bld-bridge,bld-ffi} editor/{agent_panel,bld_bridge,vscript_panel} sandbox runtime gameplay tools/cook tests content assets third_party cmake/packaging docs/{daily,snippets,architecture,adr}
```


---


---

```cpp
// snippets/ecs_system_example.cpp — Greywater_Engine
//
// The canonical ECS system shape. Systems are plain functions with declared
// read/write component sets; the scheduler runs non-conflicting systems in
// parallel without locks. This snippet demonstrates:
//
//   - Component types are plain data (no virtuals, no constructors that allocate).
//   - Queries are compile-time-typed spans over SoA-laid-out archetype chunks.
//   - Systems mutate through explicit read/write tags so the scheduler can reason.
//   - No STL in the hot path — use `gw::core::span`, not `std::span`.
//   - No raw `std::thread` — all parallelism goes through `gw::jobs`.
//
// Place actual runtime systems under `engine/<subsystem>/` and register them
// via `World::register_system<...>()` at startup.

#include <gw/core/span.hpp>
#include <gw/ecs/world.hpp>
#include <gw/math/vec.hpp>

namespace gw::gameplay {

// Plain-data components. No virtuals, no allocations, trivially-copyable.
struct Transform {
    math::Vec3 position;
    math::Quat rotation;
    math::Vec3 scale;
};

struct Velocity {
    math::Vec3 linear;
    math::Vec3 angular;
};

// A system: plain function, explicit read/write declarations via the query.
// `Write<Transform>` + `Read<Velocity>` tells the scheduler what this system
// touches so it can parallelize without locks.
void integrate_velocity(
    ecs::Query<ecs::Write<Transform>, ecs::Read<Velocity>> query,
    f32 dt) noexcept
{
    for (auto [transform, velocity] : query) {
        transform.position += velocity.linear * dt;
        // Quaternion integration via small-angle approximation omitted for brevity.
    }
}

// Registration happens once, typically at subsystem init.
inline void register_gameplay_systems(ecs::World& world)
{
    world.register_system(integrate_velocity);
}

} // namespace gw::gameplay
```


---


---

```rust
// snippets/bld_tool_example.rs — Greywater_Engine · BLD crate
//
// The canonical BLD tool shape. `#[bld_tool]` generates the MCP schema,
// JSON decoder, dispatch handler, and registry entry at compile time —
// no hand-written JSON, no libclang pass, no external codegen tool.
//
// Tools live inside the `bld-tools` crate (for engine-agnostic ones)
// or inside the specific `bld-*` sub-crate that owns the domain.
//
// The pattern:
//
//   - Tools are free functions taking typed args, returning `Result<T, E>`.
//   - The proc macro derives the MCP schema from the signature.
//   - Mutating tools must declare a `tier = "mutate"` or `tier = "execute"`;
//     `bld-governance` enforces approval at dispatch time.
//   - Callbacks into the C++ editor go through `bld_ffi::editor_callback(...)`
//     — never touch C++ state directly.
//
// Run `cargo build -p bld-tools` to regenerate the registry after adding
// or changing a tool.

use bld_ffi::editor_callback;
use bld_tools::{bld_tool, ToolError, ToolResult};
use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize)]
pub struct EntityHandle(pub u64);

#[derive(Deserialize, Serialize)]
pub struct CreateEntityArgs {
    /// Human-readable name for the new entity. Used in the editor hierarchy.
    pub name: String,
    /// Optional parent entity. When set, the new entity inherits the parent's
    /// transform space.
    pub parent: Option<EntityHandle>,
}

/// Create a new entity in the active scene.
///
/// Tier: `mutate` — requires human approval per session (or per call under
/// strict governance mode). The action is committed via the editor command
/// bus so Ctrl+Z reverts it atomically.
#[bld_tool(
    namespace = "scene",
    name = "create_entity",
    tier = "mutate",
    summary = "Create a new entity in the active scene."
)]
pub fn create_entity(args: CreateEntityArgs) -> ToolResult<EntityHandle> {
    // Submit the command through the C-ABI callback. The editor enqueues
    // it on the command stack; the returned handle is guaranteed valid
    // only after the command executes.
    let handle = editor_callback::scene_create_entity(&args.name, args.parent.as_ref())
        .map_err(ToolError::editor)?;

    Ok(EntityHandle(handle))
}

// Tests use the in-memory editor stub so CI can exercise the tool
// registry without a running editor process.
#[cfg(test)]
mod tests {
    use super::*;
    use bld_ffi::test_stub as stub;

    #[test]
    fn create_entity_returns_valid_handle() {
        stub::reset();
        let handle = create_entity(CreateEntityArgs {
            name: "TestEntity".into(),
            parent: None,
        })
        .expect("stub should succeed");
        assert_ne!(handle.0, 0);
    }
}
```