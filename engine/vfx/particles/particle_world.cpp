// engine/vfx/particles/particle_world.cpp — ADR-0077/0078 Wave 17D/E.

#include "engine/vfx/particles/particle_world.hpp"

#include "engine/core/config/cvar_registry.hpp"
#include "engine/vfx/decals/decal_renderer.hpp"
#include "engine/vfx/particles/particle_emitter.hpp"
#include "engine/vfx/particles/particle_renderer.hpp"
#include "engine/vfx/particles/particle_simulation.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>

namespace gw::vfx::particles {

namespace {
using clk = std::chrono::steady_clock;
}

struct ParticleWorld::Impl {
    ParticleWorldConfig           cfg{};
    gw::config::CVarRegistry*     cvars{nullptr};
    gw::events::CrossSubsystemBus* bus{nullptr};
    bool                          inited{false};
    std::uint64_t                 frame{0};

    std::vector<std::unique_ptr<ParticleEmitter>> emitters;          // id == index+1
    std::unordered_map<std::string, EmitterId>    emitters_by_name;

    std::vector<GpuParticle>                      particles;
    ParticleRenderer                              renderer;

    std::vector<std::unique_ptr<ribbons::RibbonState>> ribbons;      // id == index+1

    std::vector<decals::DecalVolume>              decals_vec;
    std::unique_ptr<decals::DecalBinner>          binner;

    ParticleStats stats{};

    void pull_cvars() {
        if (!cvars) return;
        cfg.particle_budget = static_cast<std::uint32_t>(
            cvars->get_i32_or("vfx.particles.budget", static_cast<std::int32_t>(cfg.particle_budget)));
        cfg.ribbon_budget = static_cast<std::uint32_t>(
            cvars->get_i32_or("vfx.ribbons.budget", static_cast<std::int32_t>(cfg.ribbon_budget)));
        cfg.decal_budget = static_cast<std::uint32_t>(
            cvars->get_i32_or("vfx.decals.budget", static_cast<std::int32_t>(cfg.decal_budget)));
        cfg.decal_tile_size = static_cast<std::uint32_t>(
            cvars->get_i32_or("vfx.decals.cluster_bin", static_cast<std::int32_t>(cfg.decal_tile_size)));
        cfg.gpu_async = cvars->get_bool_or("vfx.gpu_async", cfg.gpu_async);
        cfg.curl_noise_octaves = static_cast<std::uint32_t>(
            cvars->get_i32_or("vfx.particles.curl_noise_octaves", static_cast<std::int32_t>(cfg.curl_noise_octaves)));
        cfg.sort_mode = parse_sort_mode(
            cvars->get_string_or("vfx.particles.sort_mode", std::string{to_cstring(cfg.sort_mode)}));
    }
};

ParticleWorld::ParticleWorld() : impl_(std::make_unique<Impl>()) {}
ParticleWorld::~ParticleWorld() { shutdown(); }

bool ParticleWorld::initialize(ParticleWorldConfig cfg,
                                 config::CVarRegistry* cvars,
                                 events::CrossSubsystemBus* bus) {
    shutdown();
    impl_->cfg   = std::move(cfg);
    impl_->cvars = cvars;
    impl_->bus   = bus;
    impl_->pull_cvars();
    impl_->particles.reserve(std::min<std::uint32_t>(impl_->cfg.particle_budget, 65536u));
    impl_->binner = std::make_unique<decals::DecalBinner>(
        impl_->cfg.surface_width, impl_->cfg.surface_height, impl_->cfg.decal_tile_size);
    impl_->inited = true;
    return true;
}

void ParticleWorld::shutdown() {
    impl_->emitters.clear();
    impl_->emitters_by_name.clear();
    impl_->particles.clear();
    impl_->renderer.reset();
    impl_->ribbons.clear();
    impl_->decals_vec.clear();
    impl_->binner.reset();
    impl_->stats = {};
    impl_->inited = false;
}

void ParticleWorld::step(double dt_seconds) {
    if (!impl_->inited) return;
    ++impl_->frame;

    const auto dt = static_cast<float>(dt_seconds);

    // Emission
    for (auto& e : impl_->emitters) {
        if (!e) continue;
        const auto n = e->accrue(dt);
        if (n == 0) continue;
        const auto remaining = impl_->cfg.particle_budget - static_cast<std::uint32_t>(impl_->particles.size());
        const auto actual = std::min<std::uint32_t>(n, remaining);
        if (actual == 0) continue;
        e->emit_cpu(actual, 0xCCFFEE13ULL ^ impl_->frame, impl_->particles);
        impl_->stats.particles_emitted_total += actual;
    }

    // Simulate
    const auto t0 = clk::now();
    SimulationParams sp{};
    sp.dt_seconds          = dt;
    sp.curl_noise_amplitude = 0.5f;
    sp.curl_noise_octaves  = impl_->cfg.curl_noise_octaves;
    sp.frame_seed          = impl_->frame;
    simulate_cpu(impl_->particles, sp);

    const auto before = impl_->particles.size();
    const auto live   = compact_cpu(impl_->particles);
    impl_->stats.particles_killed_total += (before - live);
    const auto t1 = clk::now();
    impl_->stats.last_simulate_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    impl_->renderer.set_live_count(live);

    // Ribbon advance
    for (auto& r : impl_->ribbons) if (r) r->advance(dt);

    impl_->stats.emitters       = static_cast<std::uint32_t>(impl_->emitters.size());
    impl_->stats.live_particles = live;
}

EmitterId ParticleWorld::create_emitter(EmitterDesc desc) {
    if (!impl_->inited) return {};
    if (impl_->emitters.size() >= impl_->cfg.emitter_budget) return {};
    if (auto it = impl_->emitters_by_name.find(desc.name); it != impl_->emitters_by_name.end()) {
        return it->second;
    }
    const auto name = desc.name;
    auto e = std::make_unique<ParticleEmitter>(std::move(desc));
    impl_->emitters.push_back(std::move(e));
    const EmitterId id{static_cast<std::uint32_t>(impl_->emitters.size())};
    impl_->emitters_by_name[name] = id;
    return id;
}

bool ParticleWorld::destroy_emitter(EmitterId id) {
    if (id.value == 0 || id.value > impl_->emitters.size()) return false;
    if (!impl_->emitters[id.value - 1]) return false;
    const auto name = impl_->emitters[id.value - 1]->desc().name;
    impl_->emitters[id.value - 1].reset();
    impl_->emitters_by_name.erase(name);
    return true;
}

bool ParticleWorld::spawn(EmitterId id, std::uint32_t count) {
    if (id.value == 0 || id.value > impl_->emitters.size()) return false;
    auto& e = impl_->emitters[id.value - 1];
    if (!e) return false;
    const auto remaining = impl_->cfg.particle_budget - static_cast<std::uint32_t>(impl_->particles.size());
    const auto actual = std::min(count, remaining);
    e->emit_cpu(actual, 0xCAFEBABEULL ^ impl_->frame, impl_->particles);
    impl_->stats.particles_emitted_total += actual;
    return true;
}

void ParticleWorld::clear_particles() noexcept {
    impl_->particles.clear();
    impl_->renderer.set_live_count(0);
}

ribbons::RibbonId ParticleWorld::create_ribbon(ribbons::RibbonDesc d) {
    if (!impl_->inited) return {};
    if (impl_->ribbons.size() >= impl_->cfg.ribbon_budget) return {};
    impl_->ribbons.push_back(std::make_unique<ribbons::RibbonState>(d));
    return ribbons::RibbonId{static_cast<std::uint32_t>(impl_->ribbons.size())};
}

bool ParticleWorld::append_ribbon_node(ribbons::RibbonId id,
                                        const std::array<float, 3>& p,
                                        float dt_seconds) {
    if (id.value == 0 || id.value > impl_->ribbons.size()) return false;
    auto& r = impl_->ribbons[id.value - 1];
    if (!r) return false;
    r->append(p, dt_seconds);
    return true;
}

std::size_t ParticleWorld::ribbon_vertex_count(ribbons::RibbonId id) const noexcept {
    if (id.value == 0 || id.value > impl_->ribbons.size()) return 0;
    const auto& r = impl_->ribbons[id.value - 1];
    return r ? r->size() : 0;
}

decals::DecalId ParticleWorld::project_decal(const decals::DecalVolume& v) {
    if (!impl_->inited) return {};
    if (impl_->decals_vec.size() >= impl_->cfg.decal_budget) return {};
    impl_->decals_vec.push_back(v);
    const decals::DecalId id{static_cast<std::uint32_t>(impl_->decals_vec.size())};
    if (impl_->binner) impl_->binner->insert(id, v);
    return id;
}

std::size_t ParticleWorld::decal_count() const noexcept {
    return impl_->decals_vec.size();
}

std::size_t ParticleWorld::decal_bin_count() const noexcept {
    return impl_->binner ? impl_->binner->bin_count() : 0;
}

ParticleStats ParticleWorld::stats() const noexcept { return impl_->stats; }
void          ParticleWorld::pull_cvars()           { impl_->pull_cvars(); }

std::span<const GpuParticle> ParticleWorld::particles_view() const noexcept {
    return std::span<const GpuParticle>(impl_->particles.data(), impl_->particles.size());
}

const ParticleWorldConfig& ParticleWorld::config() const noexcept { return impl_->cfg; }

// ---- free helpers ---------------------------------------------------------

ParticleSortMode parse_sort_mode(const std::string& s) noexcept {
    if (s == "bitonic") return ParticleSortMode::Bitonic;
    return ParticleSortMode::None;
}

const char* to_cstring(ParticleSortMode m) noexcept {
    return m == ParticleSortMode::Bitonic ? "bitonic" : "none";
}

} // namespace gw::vfx::particles
