#pragma once
// engine/render/hal/capabilities.hpp
// RHICapabilities — feature-flag snapshot populated once per physical device.
// CLAUDE.md non-negotiable #2: Vulkan 1.2 baseline, 1.3 opportunistic.
// See docs/adr/0003-rhi-capabilities.md for the full rationale.

#include <cstdint>
#include <volk.h>

namespace gw {
namespace render {
namespace hal {

// Feature snapshot taken at device init. Fields are public so callers can
// branch on them directly — the struct is the canonical source of truth
// for "what can this GPU do". Never modify after construction.
struct RHICapabilities {
    // API version (VK_MAKE_API_VERSION encoded) as reported by the driver.
    // Use api_major() / api_minor() helpers rather than decoding manually.
    std::uint32_t api_version = 0;

    // Baseline 1.2-core features. The Tier-A renderer assumes these are
    // present; VulkanDevice fails construction if any is missing.
    bool timeline_semaphores   = false;
    bool buffer_device_address = false;
    bool descriptor_indexing   = false;

    // Vulkan 1.3 opportunistic extensions (RX 580 reports true).
    // When false, the frame graph must take the legacy sync1 /
    // render-pass path — which is a Tier-G follow-up, see
    // docs/adr/0003-rhi-capabilities.md §3 "Short term".
    bool dynamic_rendering     = false;
    bool synchronization_2     = false;

    // Tier-G features (all false on RX 580; lit up opportunistically).
    bool mesh_shaders          = false;
    bool ray_tracing_pipeline  = false;
    bool ray_query             = false;
    bool gpu_work_graphs       = false;  // Vulkan 1.4

    // VMA memory-budget integration — hook for the Phase-20 streaming
    // scheduler. Audit finding P1-2 tracks the actual wiring.
    bool ext_memory_budget     = false;

    // Device-local heap size in bytes (heap with DEVICE_LOCAL_BIT set).
    // On RX 580 8 GB this is ~8,318,523,904 (7.75 GiB after driver reserve).
    std::uint64_t total_device_local_bytes = 0;

    // Max bindless array size (from VkPhysicalDeviceDescriptorIndexingProperties).
    std::uint32_t max_descriptor_indexed_array_size = 0;

    // Convenience readers. The `has_` prefix is load-bearing — upstream
    // code reads `caps.has_sync2()` rather than `caps.synchronization_2`
    // so the call site matches the surrounding `has_*` idiom in docs/03 §4.
    [[nodiscard]] bool has_sync2()             const noexcept { return synchronization_2; }
    [[nodiscard]] bool has_dynamic_rendering() const noexcept { return dynamic_rendering; }
    [[nodiscard]] bool has_timeline_semaphores()   const noexcept { return timeline_semaphores; }
    [[nodiscard]] bool has_descriptor_indexing()   const noexcept { return descriptor_indexing; }
    [[nodiscard]] bool has_buffer_device_address() const noexcept { return buffer_device_address; }
    [[nodiscard]] bool has_mesh_shaders()          const noexcept { return mesh_shaders; }
    [[nodiscard]] bool has_ray_tracing()           const noexcept { return ray_tracing_pipeline; }
    [[nodiscard]] bool has_memory_budget()         const noexcept { return ext_memory_budget; }

    [[nodiscard]] std::uint32_t api_major() const noexcept {
        return (api_version >> 22) & 0x7Fu;
    }
    [[nodiscard]] std::uint32_t api_minor() const noexcept {
        return (api_version >> 12) & 0x3FFu;
    }
    [[nodiscard]] std::uint32_t api_patch() const noexcept {
        return api_version & 0xFFFu;
    }
};

// Probe a live VkPhysicalDevice. Never throws; missing features become
// `false` / zero. The probe itself does not enable any extension or feature
// — it only reports what the device claims to support.
[[nodiscard]] RHICapabilities probe_capabilities(VkPhysicalDevice physical_device) noexcept;

}  // namespace hal
}  // namespace render
}  // namespace gw
