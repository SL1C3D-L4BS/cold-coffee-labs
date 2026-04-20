# 03_VULKAN_RESEARCH_GUIDE — Greywater_Engine Rendering Stack

**Status:** Reference
**Scope:** Vulkan 1.2-baseline with opportunistic 1.3 features as Greywater's graphics API. Staged learning path, feature commitments, capability gates, and the shader toolchain. **Hardware baseline: AMD Radeon RX 580 8 GB** (Polaris, GCN 4.0, 2016).

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

- **GPU terrain mesh generation** — compute shaders generate spherical planet mesh chunks from the deterministic height function. See `docs/games/greywater/14_PLANETARY_SYSTEMS.md`.
- **Raymarched atmospheric scattering** — single-pass physically-based, no LUT dependency, runs cleanly on the RX 580. See `docs/games/greywater/15_ATMOSPHERE_AND_SPACE.md`.
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
