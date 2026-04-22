// tests/unit/vfx/phase17_particles_test.cpp — ADR-0077 Wave 17D.

#include <doctest/doctest.h>

#include "engine/vfx/particles/particle_emitter.hpp"
#include "engine/vfx/particles/particle_simulation.hpp"
#include "engine/vfx/particles/particle_types.hpp"
#include "engine/vfx/particles/particle_world.hpp"

#include <array>
#include <cstdint>

using namespace gw::vfx::particles;

static EmitterDesc make_desc() {
    EmitterDesc d;
    d.name             = "spark";
    d.shape            = EmitterShape::Point;
    d.spawn_rate_per_s = 120.0f;
    d.min_lifetime     = 1.0f;
    d.max_lifetime     = 2.0f;
    d.min_speed        = 0.0f;
    d.max_speed        = 1.0f;
    return d;
}

TEST_CASE("phase17.particles: GpuParticle POD is frozen at 48 B") {
    CHECK(sizeof(GpuParticle) == 48);
}

TEST_CASE("phase17.particles: emit_cpu is deterministic for equal seed") {
    ParticleEmitter e1{make_desc()};
    ParticleEmitter e2{make_desc()};
    std::vector<GpuParticle> a, b;
    const auto na = e1.emit_cpu(16, 0xABCDu, a);
    const auto nb = e2.emit_cpu(16, 0xABCDu, b);
    CHECK(na == nb);
    CHECK(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].position_x == doctest::Approx(b[i].position_x));
        CHECK(a[i].lifetime   == doctest::Approx(b[i].lifetime));
    }
}

TEST_CASE("phase17.particles: accrue produces the documented spawn count") {
    ParticleEmitter e{make_desc()};
    const auto n = e.accrue(1.0f);
    CHECK(n >= 100);
    CHECK(n <= 130);
}

TEST_CASE("phase17.particles: accrue accumulates fractional spawns") {
    ParticleEmitter e{make_desc()};
    std::uint32_t total = 0;
    for (int i = 0; i < 60; ++i) total += e.accrue(1.0f / 60.0f);
    CHECK(total >= 100);
    CHECK(total <= 130);
}

TEST_CASE("phase17.particles: simulate_cpu advances age by dt") {
    std::vector<GpuParticle> ps(4);
    for (auto& p : ps) { p.lifetime = 10.0f; }
    SimulationParams sp;
    sp.dt_seconds = 1.0f / 30.0f;
    sp.gravity = {0, 0, 0};
    simulate_cpu(ps, sp);
    for (const auto& p : ps) CHECK(p.age == doctest::Approx(1.0f / 30.0f));
}

TEST_CASE("phase17.particles: simulate_cpu applies gravity") {
    std::vector<GpuParticle> ps(1);
    ps[0].lifetime = 10.0f;
    SimulationParams sp;
    sp.dt_seconds = 1.0f;
    sp.gravity = {0, -10, 0};
    simulate_cpu(ps, sp);
    CHECK(ps[0].velocity_y == doctest::Approx(-10.0f));
}

TEST_CASE("phase17.particles: drag brings velocity toward zero") {
    std::vector<GpuParticle> ps(1);
    ps[0].lifetime = 10.0f;
    ps[0].velocity_x = 10.0f;
    SimulationParams sp;
    sp.dt_seconds = 1.0f;
    sp.gravity = {0, 0, 0};
    sp.drag = 0.5f;
    simulate_cpu(ps, sp);
    CHECK(ps[0].velocity_x < 10.0f);
    CHECK(ps[0].velocity_x > 0.0f);
}

TEST_CASE("phase17.particles: compact_cpu removes dead particles") {
    std::vector<GpuParticle> ps(4);
    ps[0].age = 0.1f; ps[0].lifetime = 1.0f;
    ps[1].age = 2.0f; ps[1].lifetime = 1.0f;   // dead
    ps[2].age = 0.1f; ps[2].lifetime = 1.0f;
    ps[3].age = 2.0f; ps[3].lifetime = 1.0f;   // dead
    const auto live = compact_cpu(ps);
    CHECK(live == 2u);
}

TEST_CASE("phase17.particles: compact_cpu with all-alive keeps every slot") {
    std::vector<GpuParticle> ps(3);
    for (auto& p : ps) { p.age = 0; p.lifetime = 10; }
    const auto live = compact_cpu(ps);
    CHECK(live == 3u);
}

TEST_CASE("phase17.particles: compact_cpu with all-dead returns zero") {
    std::vector<GpuParticle> ps(3);
    for (auto& p : ps) { p.age = 10; p.lifetime = 1; }
    const auto live = compact_cpu(ps);
    CHECK(live == 0u);
}

TEST_CASE("phase17.particles: curl_noise_sample deterministic for seed+pos") {
    const auto a = curl_noise_sample(1.0f, 2.0f, 3.0f, 3, 0x1234u);
    const auto b = curl_noise_sample(1.0f, 2.0f, 3.0f, 3, 0x1234u);
    CHECK(a[0] == doctest::Approx(b[0]));
    CHECK(a[1] == doctest::Approx(b[1]));
    CHECK(a[2] == doctest::Approx(b[2]));
}

TEST_CASE("phase17.particles: curl_noise_sample differs across seeds") {
    const auto a = curl_noise_sample(1.0f, 2.0f, 3.0f, 3, 0x1234u);
    const auto b = curl_noise_sample(1.0f, 2.0f, 3.0f, 3, 0xCAFEu);
    const bool any_differ = (a[0] != b[0]) || (a[1] != b[1]) || (a[2] != b[2]);
    CHECK(any_differ);
}

TEST_CASE("phase17.particles: sort mode parses from string") {
    CHECK(parse_sort_mode("none")    == ParticleSortMode::None);
    CHECK(parse_sort_mode("bitonic") == ParticleSortMode::Bitonic);
    CHECK(std::string{to_cstring(ParticleSortMode::None)}    == "none");
    CHECK(std::string{to_cstring(ParticleSortMode::Bitonic)} == "bitonic");
}

TEST_CASE("phase17.particles.world: initialize seeds budgets") {
    ParticleWorld w;
    ParticleWorldConfig cfg{};
    cfg.particle_budget = 128;
    cfg.emitter_budget  = 4;
    CHECK(w.initialize(cfg));
    CHECK(w.config().particle_budget == 128);
    w.shutdown();
}

TEST_CASE("phase17.particles.world: create_emitter + spawn is bounded") {
    ParticleWorld w;
    ParticleWorldConfig cfg{};
    cfg.particle_budget = 64;
    CHECK(w.initialize(cfg));
    const auto id = w.create_emitter(make_desc());
    CHECK(id.valid());
    CHECK(w.spawn(id, 1000));
    CHECK(w.particles_view().size() <= 64);
    w.shutdown();
}

TEST_CASE("phase17.particles.world: clear_particles resets view to zero") {
    ParticleWorld w;
    ParticleWorldConfig cfg{};
    cfg.particle_budget = 32;
    CHECK(w.initialize(cfg));
    const auto id = w.create_emitter(make_desc());
    (void)w.spawn(id, 16);
    w.clear_particles();
    CHECK(w.particles_view().empty());
    w.shutdown();
}
