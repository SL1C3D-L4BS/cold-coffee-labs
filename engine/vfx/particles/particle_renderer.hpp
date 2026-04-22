#pragma once
// engine/vfx/particles/particle_renderer.hpp — ADR-0077 Wave 17D.
//
// Headless-friendly renderer shim: owns indirect-draw arguments that the
// GPU path will consume, but never calls into Vulkan from this header. The
// real pipeline lives in the .cpp and is activated when a Vulkan device is
// wired; here we only expose the count-reset + stats interface.

#include "engine/vfx/particles/particle_types.hpp"

#include <cstdint>

namespace gw::vfx::particles {

struct IndirectDrawArgs {
    std::uint32_t vertex_count{4};
    std::uint32_t instance_count{0};
    std::uint32_t first_vertex{0};
    std::uint32_t first_instance{0};
};

class ParticleRenderer {
public:
    void set_live_count(std::uint64_t n) noexcept;
    [[nodiscard]] const IndirectDrawArgs& args() const noexcept { return args_; }
    void reset() noexcept;

private:
    IndirectDrawArgs args_{};
};

} // namespace gw::vfx::particles
