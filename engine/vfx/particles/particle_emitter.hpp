#pragma once
// engine/vfx/particles/particle_emitter.hpp — ADR-0077 Wave 17D.

#include "engine/vfx/particles/particle_types.hpp"

#include <cstdint>
#include <vector>

namespace gw::vfx::particles {

// An emitter owns the recipe for spawning new particles. It appends to
// a provided GpuParticle buffer (CPU-side reference; GPU variant writes
// to the SSBO — out of scope for headless tests).
class ParticleEmitter {
public:
    ParticleEmitter() = default;
    explicit ParticleEmitter(EmitterDesc d);

    [[nodiscard]] const EmitterDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] std::uint64_t      total_emitted() const noexcept { return total_emitted_; }
    [[nodiscard]] std::uint64_t      live_count() const noexcept { return live_count_; }

    // Deterministic emit (seed is explicit).
    std::uint32_t emit_cpu(std::uint32_t n,
                            std::uint64_t seed,
                            std::vector<GpuParticle>& buffer);

    // Accrue time; returns the number of particles the emitter wants to
    // spawn this tick based on spawn_rate_per_s and dt.
    [[nodiscard]] std::uint32_t accrue(float dt_seconds);

    void set_live_count(std::uint64_t c) noexcept { live_count_ = c; }

private:
    EmitterDesc   desc_{};
    float         spawn_accumulator_{0.0f};
    std::uint64_t total_emitted_{0};
    std::uint64_t live_count_{0};
};

} // namespace gw::vfx::particles
