#pragma once
// engine/render/hal/present_timing.hpp — Part C §21 scaffold.
//
// VK_KHR_present_id + VK_KHR_present_wait frame pacing + latency metrics.
// Results published via the existing telemetry surface.

#include <cstdint>

namespace gw::render::hal {

struct PresentTiming {
    std::uint64_t present_id            = 0;
    std::uint64_t gpu_timestamp_ns      = 0;
    std::uint64_t cpu_present_ns        = 0;
    float         latency_ms_cpu_to_gpu = 0.f;
    float         latency_ms_cpu_to_display = 0.f;
};

/// Query latest present timing from VK_KHR_present_wait. Returns false if
/// the extensions are not supported on the active device.
bool sample_present_timing(PresentTiming& out) noexcept;

} // namespace gw::render::hal
