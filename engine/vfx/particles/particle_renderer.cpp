// engine/vfx/particles/particle_renderer.cpp — ADR-0077 Wave 17D.

#include "engine/vfx/particles/particle_renderer.hpp"

#include <algorithm>
#include <limits>

namespace gw::vfx::particles {

void ParticleRenderer::set_live_count(std::uint64_t n) noexcept {
    args_.instance_count = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(n, std::numeric_limits<std::uint32_t>::max()));
}

void ParticleRenderer::reset() noexcept {
    args_ = IndirectDrawArgs{};
}

} // namespace gw::vfx::particles
