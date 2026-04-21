# ADR 0003 — RHI Capabilities layer

- **Status:** Accepted (amended 2026-04-20 late-night — see §2.1)
- **Date:** 2026-04-20
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiable #2 (Vulkan 1.2 baseline, 1.3 opportunistic); `docs/00_SPECIFICATION_Claude_Opus_4.7.md §2.2`; `docs/03_VULKAN_RESEARCH_GUIDE.md §4`; `docs/AUDIT_MAP_2026-04-20.md §P1-1`.

## 1. Context

`CLAUDE.md` non-negotiable #2 says:

> **Vulkan 1.2 baseline** with **1.3 features opportunistic** (via `RHICapabilities`).

`docs/03 §4` sketches the `struct RHICapabilities` shape. The reference docs have consistently promised a single feature-flag struct populated at device init; callers gate on booleans; the frame graph picks code paths based on capability (no runtime `if (mesh_shaders) …` strewn through shading code).

In reality (end-of-day 2026-04-20), no such symbol exists. `engine/render/hal/vulkan_device.cpp` unconditionally enables `VK_KHR_dynamic_rendering` + `VK_KHR_synchronization2`, unconditionally chains the corresponding feature structs into `VkPhysicalDeviceFeatures2.pNext`, and unconditionally fails device creation if the physical device does not support them. `engine/render/frame_graph/*` records via `vkCmdPipelineBarrier2` and `vkCmdBeginRendering` with no query against what the device can do. The sandbox, editor, and render paths all assume 1.3 features.

This is operationally fine on the reference RX 580 workstation (driver reports `apiVersion = 1.3.260` with both extensions) and is the root reason Round-1 audit items did not trip during evening-session verification. It is structurally incorrect and blocks Phase 8:

1. Cross-device CI (Phase 8 Linux matrix) will expose any driver that reports a bare 1.2 core with no extensions and will hard-fail device creation rather than degrading.
2. Tier-G gates referenced all over `docs/02_SYSTEMS_AND_SIMULATIONS.md`, `docs/10 §3`, and `docs/03 §4` have no code-level anchor to compile against.
3. Future features (mesh shaders, ray tracing, VK 1.4 work graphs — all Tier G per `docs/03 §1.2`) need a named place to land.

## 2. Decision

Introduce `engine/render/hal/capabilities.hpp` (+ `.cpp`) exposing:

```cpp
namespace gw::render::hal {

struct RHICapabilities {
    // API version (from VkPhysicalDeviceProperties.apiVersion)
    std::uint32_t api_version = 0;

    // Baseline 1.2-core features (expected true on every shipping target)
    bool timeline_semaphores    = false;
    bool buffer_device_address  = false;
    bool descriptor_indexing    = false;  // bindless

    // Vulkan 1.3 opportunistic (RX 580 reports true; older GCN may not)
    bool dynamic_rendering      = false;
    bool synchronization_2      = false;

    // Tier G — hardware-gated (all false on RX 580)
    bool mesh_shaders           = false;
    bool ray_tracing_pipeline   = false;
    bool ray_query              = false;
    bool gpu_work_graphs        = false;  // Vulkan 1.4

    // Memory-budget integration for streaming scheduler (P1-2)
    bool ext_memory_budget      = false;

    // Device properties surfaced for callers that need them
    std::uint64_t total_device_local_bytes = 0;
    std::uint32_t max_descriptor_indexed_array_size = 0;

    // Convenience queries (read-only; do not mutate)
    [[nodiscard]] bool has_sync2()             const noexcept { return synchronization_2; }
    [[nodiscard]] bool has_dynamic_rendering() const noexcept { return dynamic_rendering; }
    [[nodiscard]] std::uint32_t api_major()    const noexcept;
    [[nodiscard]] std::uint32_t api_minor()    const noexcept;
};

// Populates every field from a live VkPhysicalDevice. Never throws; missing
// features are reported as `false`.
[[nodiscard]] RHICapabilities probe_capabilities(VkPhysicalDevice) noexcept;

} // namespace gw::render::hal
```

`VulkanDevice` probes once at construction and stores the result. Two consequences follow:

1. **Device creation becomes capability-aware.** The device-extension list and the feature-struct chain are built from the probe result, not from a hard-coded list. Missing optional features are reported as `false` in `RHICapabilities` and the corresponding extension is simply not enabled; missing *baseline* features (timeline semaphores, descriptor indexing, synchronization_2, dynamic_rendering — see §2.1) remain a hard error.

2. **Higher layers branch on capabilities, never on extension names.** The frame graph reads `device.capabilities()` — e.g. `has_mesh_shaders()`, `has_ray_tracing()` — when deciding which code path to emit. For Tier-A features the branch is compiled out (the capability is guaranteed present); for Tier-G features the branch is the single gating point for optional code paths. What this ADR **does** mandate is that the branch point exists, is one line of code, and lives at the frame-graph level (not at the HAL level).

## 2.1. Amendment (2026-04-20 late-night) — `synchronization_2` and `dynamic_rendering` promoted to Tier-A

**Context.** The original §2 framing had `synchronization_2` and `dynamic_rendering` as *opportunistic* (Tier-B): the probe would report presence, device creation would enable them when available, and the frame graph would branch on `has_sync2()` / `has_dynamic_rendering()`. The first-pass implementation made device creation capability-aware correctly, **but the frame-graph branch was a silent no-op** — `execute_pass` would skip `vkCmdPipelineBarrier2` entirely when `has_sync2()` was false. That is a silent correctness hole (dropped barriers → wrong-stage submission, memory-ordering bugs visible only under load), not a fallback.

**Reality check.** A real fallback would need:

1. A sync1 barrier-emit path (translation tables for stage+access masks per Vulkan spec §7.4.2).
2. A legacy `VkRenderPass` + `VkFramebuffer` builder for the frame graph's render targets, replacing `vkCmdBeginRendering` / `vkCmdEndRendering`.
3. A second code path through `engine/render/frame_graph/command_buffer.cpp` (the Tier-A wrapper today calls the sync2/dynamic-rendering intrinsics directly).
4. Cross-hardware CI (lavapipe, MoltenVK, pre-1.3 AMD/NVIDIA) to exercise it.

**Decision.** Promote both features to Tier-A:

- `VulkanDevice::VulkanDevice` refuses to construct when `!caps.has_sync2()` or `!caps.has_dynamic_rendering()`, with a specific error message that points to this ADR.
- The feature-struct chain in device creation unconditionally enables both (no `if (caps_.has_sync2())` guards in device init).
- The frame-graph barrier emit drops its `has_sync2()` guard; the capability is a construction invariant.
- `capabilities().has_sync2()` / `has_dynamic_rendering()` remain as queries, for code that wants to assert or document the invariant, but callers may assume true on any successfully-constructed `VulkanDevice`.

**Tier-G follow-up.** A real sync1 + legacy-render-pass backend becomes a *phase task*, not a lurking correctness hole. When the matrix expands (lavapipe for Linux CI without a real GPU; MoltenVK for macOS — neither of these advertises sync2 today), reopen this ADR with the additional scope and revert `has_sync2()` to opportunistic. Until then, the reference hardware (RX 580, apiVersion 1.3.260) and every Phase-7-target driver advertises both, so nothing is actually being given up.

**What this doesn't change.** NN #2's "Vulkan 1.2 baseline; 1.3 opportunistic" remains correct at the *feature category* level — we still compile against 1.2 core, `VkInstance` is created with `apiVersion = 1.2`, and Tier-G features (mesh shaders, ray tracing, work graphs, `VK_EXT_memory_budget`) stay strictly opportunistic. The amendment only promotes two extensions from Tier-B to Tier-A based on their actual usage pattern in our codebase.

## 3. Consequences

### Immediate (this commit + the 2026-04-20 late-night amendment)
- `VulkanDevice::capabilities()` returns `const RHICapabilities&` — callers can read but not mutate.
- `VulkanDevice` fails loudly at construction if *baseline* capabilities are missing. Baseline = timeline_semaphores, descriptor_indexing, synchronization_2, dynamic_rendering (post-amendment §2.1). Missing Tier-B / Tier-G capabilities are reported as `false` in `caps_` and produce no error.
- The frame-graph barrier emit dropped its `has_sync2()` guard; sync2 presence is a `VulkanDevice` construction invariant.
- Existing call sites (`sandbox/main.cpp`, `editor/app/editor_app.cpp`) continue to work — they construct their own Vulkan device / instance and do not route through `VulkanDevice`, so their status is unchanged. A follow-up task (tracked against Phase 8 week 043) will consolidate onto `VulkanDevice`; see `docs/AUDIT_MAP_2026-04-20.md §8`.

### Short term (Phase 8 week 043–045)
- `editor/app/editor_app.cpp` and `sandbox/main.cpp` migrate to `VulkanDevice` so they too inherit the capability probe.
- `P1-2` (`VK_EXT_memory_budget` integration) lands on top of `ext_memory_budget`.
- Reopen §2.1 if (and only if) we add cross-hardware CI that doesn't advertise sync2 or dynamic_rendering. That reopening is the one trigger that makes the sync1 + legacy-render-pass backend a Tier-G work item.

### Long term
- Tier-G features (mesh shaders, ray tracing, work graphs) each add one boolean to `RHICapabilities` and one branch point in the frame graph. No other subsystem touches these strings directly.
- Validation layers + capability probe combine to produce a deterministic cross-device test matrix — every CI runner reports its `RHICapabilities` into the run log, and divergence is a ticketable event rather than a "works on my machine" mystery.

### Alternatives considered

1. **Inline checks at every call site.** Rejected: this is exactly the "`if (mesh_shaders) …` strewn through shading code" pattern `docs/03 §4` tells us to avoid.

2. **Runtime dispatch tables (pimpl-per-feature).** Rejected for now as premature. The number of optional features is small enough that a flat struct of booleans is simpler, and the cost of a branch in the recording path is dominated by the `vkCmd…` call overhead anyway.

3. **Extension-string-based queries at the call site.** Rejected: strings are typo-prone (audit finding P1-3 was exactly this — `VK_EXT_dynamic_rendering` instead of `VK_KHR_dynamic_rendering`). Capabilities are the single source of truth; strings live only inside the probe.

## 4. References

- Vulkan spec — [VkPhysicalDeviceProperties](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties.html)
- [VK_KHR_synchronization2](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_synchronization2.html)
- [VK_KHR_dynamic_rendering](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_dynamic_rendering.html)
- [VK_EXT_memory_budget](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_memory_budget.html)
- `docs/03_VULKAN_RESEARCH_GUIDE.md §4` — capability gates
- `docs/AUDIT_MAP_2026-04-20.md §P1-1` — finding
- `CLAUDE.md` non-negotiable #2

---

*Drafted by Claude Opus 4.7 on 2026-04-20 as part of the audit Round-1 tail.
Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*
