#pragma once
// engine/vfx/particles/particle_simulation.hpp — ADR-0077 Wave 17D.

#include "engine/vfx/particles/particle_types.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace gw::vfx::particles {

struct SimulationParams {
    float                dt_seconds{1.0f / 60.0f};
    std::array<float, 3> gravity{0.0f, -9.81f, 0.0f};
    float                drag{0.0f};
    float                curl_noise_amplitude{0.0f};   // ADR-0077 §5 curl-noise
    std::uint32_t        curl_noise_octaves{3};
    std::uint64_t        frame_seed{0};
};

// CPU reference path — deterministic (bit-equivalent across vendors) so the
// GPU path can be regression-tested against it. Updates ages, applies velocity
// and gravity + drag + curl-noise. Does NOT compact yet.
void simulate_cpu(std::vector<GpuParticle>& particles, const SimulationParams& params) noexcept;

// Stream-compact in place: retain alive particles (age < lifetime), move dead
// to the tail, and return the new live count. Stable order is NOT guaranteed
// (swap-remove), mirroring the GPU compaction pass.
[[nodiscard]] std::uint64_t compact_cpu(std::vector<GpuParticle>& particles) noexcept;

// Deterministic curl-noise (domain-warped value noise) used by simulate_cpu.
[[nodiscard]] std::array<float, 3>
curl_noise_sample(float x, float y, float z,
                  std::uint32_t octaves, std::uint64_t seed) noexcept;

} // namespace gw::vfx::particles
