#pragma once
// engine/vfx/particles/particle_types.hpp — ADR-0077 Wave 17D.

#include <array>
#include <cstdint>
#include <string>

namespace gw::vfx::particles {

// ---- handles --------------------------------------------------------------

struct EmitterId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const EmitterId&) const noexcept = default;
};

// ---- POD particle (48 bytes, frozen) -------------------------------------

struct alignas(16) GpuParticle {
    float position_x{0}, position_y{0}, position_z{0};
    float age{0};
    float velocity_x{0}, velocity_y{0}, velocity_z{0};
    float lifetime{0};
    float size_x{1};    float size_y{1};
    float rotation{0};
    std::uint32_t color_rgba{0xFFFFFFFFu};
};
static_assert(sizeof(GpuParticle) == 48, "GpuParticle is 48 B frozen (ADR-0077 §3)");

// ---- emitter descriptor --------------------------------------------------

enum class EmitterShape : std::uint8_t {
    Point  = 0,
    Sphere = 1,
    Box    = 2,
    Cone   = 3,
};

struct EmitterDesc {
    std::string    name;
    EmitterShape   shape{EmitterShape::Point};
    float          spawn_rate_per_s{100.0f};
    float          min_lifetime{1.0f};
    float          max_lifetime{2.0f};
    float          min_speed{0.0f};
    float          max_speed{1.0f};
    std::array<float, 3> position{0, 0, 0};
    std::array<float, 3> shape_extent{1, 1, 1};   // radius / half-extents / cone params
    std::uint32_t  initial_color_rgba{0xFFFFFFFFu};
    bool           billboard{true};
};

// ---- sort mode ------------------------------------------------------------

enum class ParticleSortMode : std::uint8_t { None = 0, Bitonic = 1 };

[[nodiscard]] ParticleSortMode parse_sort_mode(const std::string&) noexcept;
[[nodiscard]] const char*      to_cstring(ParticleSortMode) noexcept;

// ---- stats ---------------------------------------------------------------

struct ParticleStats {
    std::uint32_t emitters{0};
    std::uint64_t live_particles{0};
    std::uint64_t particles_emitted_total{0};
    std::uint64_t particles_killed_total{0};
    double        last_simulate_ms{0.0};
    double        last_render_ms{0.0};
};

} // namespace gw::vfx::particles
