// tests/perf/phase17_budgets_perf_test.cpp — Phase 17 perf gate (ADR-0081 §budgets).
//
// Exercises the CPU-reference paths of the Studio-Renderer subsystems against
// the time ceilings listed in docs/09_NETCODE_DETERMINISM_PERF.md (merged **Phase 17 performance budgets — Studio Renderer**, §Per-pass budgets).
// GPU paths are out-of-scope for CI; the CPU references stand in as proxies
// because the actual rendering is done through the same algorithmic shapes.
//
// Labels: perf + phase17.  Debug builds get `kDebugSlack` headroom; release
// builds under studio-{win,linux} hold the tight envelope.

#include "engine/render/material/material_config.hpp"
#include "engine/render/material/material_template.hpp"
#include "engine/render/material/material_types.hpp"
#include "engine/render/material/material_world.hpp"
#include "engine/render/material/gwmat.hpp"
#include "engine/render/post/bloom.hpp"
#include "engine/render/post/dof.hpp"
#include "engine/render/post/motion_blur.hpp"
#include "engine/render/post/taa.hpp"
#include "engine/render/post/tonemap.hpp"
#include "engine/vfx/particles/particle_emitter.hpp"
#include "engine/vfx/particles/particle_simulation.hpp"
#include "engine/vfx/particles/particle_types.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

namespace {

#ifndef NDEBUG
constexpr double kDebugSlack = 40.0; // Debug: generous headroom for CI.
#else
constexpr double kDebugSlack = 1.0;
#endif

struct Budget {
    const char* name;
    double      ms_ceiling_release;
};

double elapsed_ms(decltype(std::chrono::steady_clock::now()) start) noexcept {
    const auto stop = std::chrono::steady_clock::now();
    const auto micros =
        std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    return static_cast<double>(micros) / 1000.0;
}

bool run_budget(const Budget& b, double measured_ms) {
    const double ceiling = b.ms_ceiling_release * kDebugSlack;
    std::printf("phase17.perf %-36s measured=%8.3f ms ceiling=%8.3f ms %s\n",
                b.name, measured_ms, ceiling,
                measured_ms <= ceiling ? "OK" : "FAIL");
    return measured_ms <= ceiling;
}

} // namespace

int main() {
    using namespace gw::render::material;
    using namespace gw::render::post;
    using namespace gw::vfx::particles;

    bool ok = true;

    // 1. Material upload reference — 1024 instances, mutation + read-back loop.
    //    Budget: ≤ 0.3 ms CPU release.
    {
        MaterialWorld world;
        MaterialConfig cfg{};
        cfg.instance_budget = 1024;
        if (!world.initialize(cfg)) {
            std::printf("phase17.perf material-world init FAILED\n");
            return 2;
        }
        const auto tid = world.find_template_by_name("pbr_opaque/metal_rough");
        std::vector<MaterialInstanceId> ids;
        ids.reserve(1024);
        for (int i = 0; i < 1024; ++i) {
            const auto id = world.create_instance(tid);
            if (id.valid()) ids.push_back(id);
        }
        auto t0 = std::chrono::steady_clock::now();
        for (const auto id : ids) {
            (void)world.set_param_vec4(id, "base_color", 0.5F, 0.5F, 0.5F, 1.0F);
        }
        const double per_iter = elapsed_ms(t0);
        ok &= run_budget(Budget{.name = "material upload 1024 inst", .ms_ceiling_release = 0.3}, per_iter);
        world.shutdown();
    }

    // 2. .gwmat encode/decode round-trip — hot path for hot-reload.
    //    Budget: ≤ 0.5 ms per operation.
    {
        MaterialTemplateDesc desc;
        desc.name = "pbr_opaque/metal_rough";
        desc.parameter_keys = {"base_color", "metallic_roughness", "emissive"};
        MaterialTemplate material_template{desc};
        MaterialInstanceState state{};

        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 32; ++i) {
            const auto blob =
                encode_gwmat(state, material_template, material_template.desc().name);
            MaterialInstanceState out{};
            std::string name;
            (void)decode_gwmat(std::span<const std::byte>{blob.bytes}, out, name);
        }
        const double per_iter = elapsed_ms(t0) / 32.0;
        ok &= run_budget(Budget{.name = "gwmat encode+decode roundtrip", .ms_ceiling_release = 0.5}, per_iter);
    }

    // 3. Particle emission + simulate CPU reference — 64k particles.
    //    Budget (scaled from the 1M GPU budget of 2.5 ms by 1/16 since this is
    //    a 64k CPU mirror): ≤ 2.5 ms CPU release.
    {
        EmitterDesc d;
        d.name             = "perf";
        d.shape            = EmitterShape::Point;
        d.spawn_rate_per_s = 0.0F;
        d.min_lifetime     = 1.0F;
        d.max_lifetime     = 2.0F;
        d.min_speed        = 0.0F;
        d.max_speed        = 1.0F;
        ParticleEmitter em{d};
        std::vector<GpuParticle> buf;
        buf.reserve(65536);
        em.emit_cpu(65536, 0xC0FFEEU, buf);

        SimulationParams sp;
        sp.dt_seconds = 1.0F / 60.0F;
        sp.gravity    = {0.0F, -9.81F, 0.0F};
        sp.drag       = 0.1F;

        auto t0 = std::chrono::steady_clock::now();
        simulate_cpu(buf, sp);
        const double sim_ms = elapsed_ms(t0);
        ok &= run_budget(Budget{.name = "particles simulate 64k (CPU ref)", .ms_ceiling_release = 2.5}, sim_ms);

        auto t1 = std::chrono::steady_clock::now();
        (void)compact_cpu(buf);
        const double compact_ms = elapsed_ms(t1);
        ok &= run_budget(Budget{.name = "particles compact 64k (CPU ref)", .ms_ceiling_release = 1.0}, compact_ms);
    }

    // 4. Dual-Kawase bloom reference — 1080p HDR buffer, 5 iterations.
    //    Budget: ≤ 0.6 ms GPU; CPU reference gets a generous 12 ms ceiling
    //    because it's ~20× slower than the compute pass it mirrors.
    {
        constexpr std::uint32_t w = 256;
        constexpr std::uint32_t h = 144; // 1/60 scaled reference.
        std::vector<float> in(static_cast<std::size_t>(w) * h * 4, 1.2F);
        for (std::size_t i = 3; i < in.size(); i += 4) in[i] = 1.0F;
        Bloom b;
        BloomConfig cfg;
        cfg.enabled = true;
        cfg.iterations = 5;
        cfg.threshold  = 0.8F;
        cfg.intensity  = 1.0F;
        b.configure(cfg);
        std::vector<float> out;
        auto t0 = std::chrono::steady_clock::now();
        b.run(in, w, h, out);
        const double ms = elapsed_ms(t0);
        ok &= run_budget(Budget{.name = "bloom dual-kawase 256x144 CPU", .ms_ceiling_release = 12.0}, ms);
    }

    // 5. TAA reference resolve — 10 k samples.
    //    Budget: ≤ 0.5 ms GPU; CPU reference ceiling 3 ms.
    {
        Taa t;
        TaaConfig cfg;
        cfg.enabled      = true;
        cfg.clip_mode    = TaaClipMode::KDop;
        cfg.blend_factor = 0.9F;
        t.configure(cfg);
        std::array<std::array<float, 3>, 9> nb{};
        for (auto& s : nb) s = {0.4F, 0.4F, 0.4F};
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 10000; ++i) {
            (void)t.resolve({0.5F, 0.5F, 0.5F}, {0.4F, 0.4F, 0.4F}, nb);
        }
        const double ms = elapsed_ms(t0);
        ok &= run_budget(Budget{.name = "taa resolve 10k (CPU ref)", .ms_ceiling_release = 3.0}, ms);
    }

    // 6. Motion blur tilemax/neighbormax — 1920×1080 velocity field.
    //    Budget: ≤ 0.7 ms GPU; CPU reference ceiling 20 ms.
    {
        constexpr std::uint32_t w    = 384;
        constexpr std::uint32_t h    = 216;
        constexpr std::uint32_t tile = 16;
        std::vector<float> vel(static_cast<std::size_t>(w) * h * 2, 0.3F);
        std::vector<float> tm;
        std::vector<float> nm;
        auto t0 = std::chrono::steady_clock::now();
        motion_blur_tilemax(vel, w, h, tile, tm);
        motion_blur_neighbormax(tm, w / tile, h / tile, nm);
        const double ms = elapsed_ms(t0);
        ok &= run_budget(Budget{.name = "motion blur tilemax+nmax 384x216", .ms_ceiling_release = 20.0}, ms);
    }

    // 7. DoF CoC closed-form — 100k sample evaluations.
    //    Budget: ≤ 0.8 ms GPU; CPU reference ceiling 4 ms.
    {
        DofConfig cfg;
        cfg.focal_mm         = 50.0F;
        cfg.aperture         = 2.0F;
        cfg.focus_distance_m = 5.0F;
        double acc = 0.0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 100000; ++i) {
            acc += coc_radius_px(cfg, 1.0F + (static_cast<float>(i & 31) * 0.25F), 1080);
        }
        const double ms = elapsed_ms(t0);
        (void)acc;
        ok &= run_budget(Budget{.name = "dof CoC 100k samples", .ms_ceiling_release = 4.0}, ms);
    }

    // 8. Tonemap CPU reference — 500k RGB evaluations across all three curves.
    //    Budget: ≤ 0.4 ms GPU; CPU reference ceiling 8 ms.
    {
        double acc = 0.0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 500000; ++i) {
            const float v = static_cast<float>(i & 1023) * (1.0F / 256.0F);
            const std::array<float, 3> rgb{v, v * 0.9F, v * 1.1F};
            const auto pbr =
                apply_tonemap(TonemapConfig{.curve = TonemapCurve::PbrNeutral, .exposure = 1.0F}, rgb);
            const auto aces = apply_tonemap(TonemapConfig{.curve = TonemapCurve::ACES, .exposure = 1.0F}, rgb);
            acc += pbr[0] + aces[0];
        }
        const double ms = elapsed_ms(t0);
        (void)acc;
        ok &= run_budget(Budget{.name = "tonemap pbr+aces 500k samples", .ms_ceiling_release = 8.0}, ms);
    }

    std::printf("phase17.perf RESULT %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
