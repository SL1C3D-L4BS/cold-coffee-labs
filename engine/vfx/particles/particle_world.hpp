#pragma once
// engine/vfx/particles/particle_world.hpp — ADR-0077/0078 Wave 17D/E PIMPL facade.

#include "engine/vfx/particles/particle_types.hpp"
#include "engine/vfx/ribbons/ribbon_state.hpp"
#include "engine/vfx/decals/decal_volume.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::config  { class CVarRegistry; }
namespace gw::events  { class CrossSubsystemBus; }
namespace gw::console { class ConsoleService; }

namespace gw::vfx::particles {

struct ParticleWorldConfig {
    std::uint32_t particle_budget{1u << 20};     // 1 M, matches ADR-0077
    std::uint32_t emitter_budget{64};
    std::uint32_t ribbon_budget{128};
    std::uint32_t decal_budget{512};
    bool          gpu_async{false};
    std::uint32_t curl_noise_octaves{3};
    std::uint32_t decal_tile_size{16};
    std::uint32_t surface_width{1920};
    std::uint32_t surface_height{1080};
    ParticleSortMode sort_mode{ParticleSortMode::None};
};

// Facade that owns all VFX worlds: particles + ribbons + decals.
class ParticleWorld {
public:
    ParticleWorld();
    ~ParticleWorld();
    ParticleWorld(const ParticleWorld&)            = delete;
    ParticleWorld& operator=(const ParticleWorld&) = delete;
    ParticleWorld(ParticleWorld&&)                 = delete;
    ParticleWorld& operator=(ParticleWorld&&)      = delete;

    [[nodiscard]] bool initialize(ParticleWorldConfig cfg,
                                   config::CVarRegistry* cvars = nullptr,
                                   events::CrossSubsystemBus* bus = nullptr);
    void               shutdown();

    void step(double dt_seconds);

    // ---- Emitters -------------------------------------------------------
    [[nodiscard]] EmitterId create_emitter(EmitterDesc desc);
    bool                    destroy_emitter(EmitterId id);
    bool                    spawn(EmitterId id, std::uint32_t count);
    void                    clear_particles() noexcept;

    // ---- Ribbons --------------------------------------------------------
    [[nodiscard]] ribbons::RibbonId create_ribbon(ribbons::RibbonDesc d);
    bool                             append_ribbon_node(ribbons::RibbonId id,
                                                        const std::array<float, 3>& p,
                                                        float dt_seconds);
    [[nodiscard]] std::size_t        ribbon_vertex_count(ribbons::RibbonId id) const noexcept;

    // ---- Decals ---------------------------------------------------------
    [[nodiscard]] decals::DecalId project_decal(const decals::DecalVolume& v);
    [[nodiscard]] std::size_t     decal_count() const noexcept;
    [[nodiscard]] std::size_t     decal_bin_count() const noexcept;

    // ---- Stats + CVars --------------------------------------------------
    [[nodiscard]] ParticleStats        stats() const noexcept;
    void                               pull_cvars();

    // ---- Testing hooks --------------------------------------------------
    [[nodiscard]] std::span<const GpuParticle> particles_view() const noexcept;
    [[nodiscard]] const ParticleWorldConfig&    config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::vfx::particles
