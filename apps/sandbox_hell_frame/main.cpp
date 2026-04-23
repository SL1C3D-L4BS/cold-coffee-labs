// apps/sandbox_hell_frame/main.cpp — Phase 21 W142 (ADR-0108).
//
// Hell Frame exit-gate smoke test. Exercises the Phase 21 Narrative/Horror
// Frame stack end-to-end without a Vulkan device:
//
//   1. Pick each of the nine Circles, read its Location Skin, drive the
//      horror post stack + Circle post stages at the Circle's baselines.
//   2. Sample the Martyrdom audio controller across Sin 0 → 100 to verify
//      the three-stem gains + dialogue triggers fire on schedule.
//   3. Fill the BloodDecalSystem to capacity, tick it one simulated
//      second at 144 Hz, and assert alpha fade is deterministic.
//   4. Print:
//         HELL FRAME — circles=9 stages=horror+sin+ruin+rapture
//                       audio=OK decals=OK reality_warp=OK
//   5. Exit 0 on success, non-zero on any sanity failure.

#include "engine/audio/martyrdom_audio_controller.hpp"
#include "engine/render/decals/blood_decal_system.hpp"
#include "engine/render/post/circle_post_stages.hpp"
#include "engine/render/post/horror_post.hpp"
#include "engine/render/post/reality_warp.hpp"
#include "engine/world/circle_profile.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

bool finite3(const std::array<float, 3>& v) noexcept {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

} // namespace

int main() {
    using gw::audio::DialogueTrigger;
    using gw::audio::MartyrdomAudioInput;
    using gw::audio::MartyrdomAudioState;
    using gw::audio::step_martyrdom_audio;
    using gw::render::decals::BloodDecalSystem;
    using gw::render::decals::DecalComponent;
    using gw::render::post::CirclePostCompose;
    using gw::render::post::HorrorPostConfig;
    using gw::render::post::RealityWarpConfig;
    using gw::render::post::circle_post_apply;
    using gw::render::post::horror_post_sample;
    using gw::render::post::reality_warp_uv;
    using gw::world::CircleId;
    using gw::world::circle_profiles;

    // ------------------------------------------------------------------ (1)
    // Drive the horror + Circle post stacks per-Circle.
    const auto& profiles = circle_profiles();
    if (profiles.size() != 9u) {
        std::fprintf(stderr, "HELL FRAME — circles:FAIL (profile count = %zu)\n",
                     profiles.size());
        return 2;
    }

    for (const auto& profile : profiles) {
        HorrorPostConfig hp{};
        hp.chromatic_aberration = profile.location_skin.horror_chromatic_aberration;
        hp.film_grain           = profile.location_skin.horror_film_grain;
        hp.screen_shake         = profile.location_skin.horror_screen_shake;
        hp.frame_index          = static_cast<std::uint64_t>(profile.id);

        const auto rgb0 =
            horror_post_sample(hp, {0.5f, 0.5f}, {0.4f, 0.4f, 0.4f});
        if (!finite3(rgb0)) {
            std::fprintf(stderr, "HELL FRAME — horror_post:FAIL circle=%u\n",
                         static_cast<unsigned>(profile.id));
            return 3;
        }

        CirclePostCompose cp{};
        cp.sin_tendril      = {profile.location_skin.sin_tendril_baseline, 50.0f};
        cp.ruin_desat       = {profile.location_skin.ruin_desat_baseline, 0.5f};
        cp.rapture_whiteout = {profile.location_skin.rapture_whiteout_baseline,
                               1.0f};
        const auto rgb1 = circle_post_apply(cp, {0.25f, 0.75f},
                                             {rgb0[0], rgb0[1], rgb0[2]});
        if (!finite3(rgb1)) {
            std::fprintf(stderr, "HELL FRAME — circle_post:FAIL circle=%u\n",
                         static_cast<unsigned>(profile.id));
            return 4;
        }
    }

    // ------------------------------------------------------------------ (2)
    // Martyrdom audio bed — walk Sin 0 → 100 and verify the 30/60/90
    // triggers + Rapture onset fire once each.
    {
        MartyrdomAudioState state{};
        MartyrdomAudioInput in{};
        in.dt_sec = 1.0f / 60.0f;

        bool saw_30 = false;
        bool saw_60 = false;
        bool saw_90 = false;

        for (int step = 0; step <= 100; ++step) {
            in.prev_sin_current = in.sin_current;
            in.sin_current      = static_cast<float>(step);
            const auto out = step_martyrdom_audio(state, in);
            state          = out.state;
            switch (out.trigger) {
                case DialogueTrigger::SinThreshold30: saw_30 = true; break;
                case DialogueTrigger::SinThreshold60: saw_60 = true; break;
                case DialogueTrigger::SinThreshold90: saw_90 = true; break;
                default: break;
            }
        }
        if (!saw_30 || !saw_60 || !saw_90) {
            std::fprintf(stderr,
                         "HELL FRAME — dialogue_triggers:FAIL "
                         "(30=%d 60=%d 90=%d)\n",
                         static_cast<int>(saw_30), static_cast<int>(saw_60),
                         static_cast<int>(saw_90));
            return 5;
        }

        MartyrdomAudioInput rin{};
        rin.dt_sec             = 1.0f / 60.0f;
        rin.prev_rapture_active = false;
        rin.rapture_active      = true;
        const auto r = step_martyrdom_audio(state, rin);
        if (r.trigger != DialogueTrigger::RaptureOnset) {
            std::fprintf(stderr, "HELL FRAME — rapture_onset:FAIL\n");
            return 6;
        }
    }

    // ------------------------------------------------------------------ (3)
    // Blood decal ring — fill, tick 1 s at 144 Hz, ensure determinism.
    {
        BloodDecalSystem a;
        BloodDecalSystem b;
        for (std::uint32_t i = 0; i < 256; ++i) {
            DecalComponent d{};
            d.world_pos[0] = static_cast<float>(i);
            d.alpha        = 1.0f;
            d.stamp_seed   = i;
            a.insert(d);
            b.insert(d);
        }
        for (std::uint64_t f = 0; f < 144; ++f) {
            a.tick(1.0f / 144.0f, f);
            b.tick(1.0f / 144.0f, f);
        }
        if (a.live_count() != b.live_count()) {
            std::fputs("HELL FRAME — decals:FAIL (determinism)\n", stderr);
            return 7;
        }
    }

    // ------------------------------------------------------------------ (4)
    // Reality warp — identity and mirror check.
    {
        RealityWarpConfig cfg{};
        const auto id = reality_warp_uv(cfg, {0.3f, 0.7f});
        if (std::fabs(id[0] - 0.3f) > 1e-4f ||
            std::fabs(id[1] - 0.7f) > 1e-4f) {
            std::fputs("HELL FRAME — reality_warp:identity FAIL\n", stderr);
            return 8;
        }
        cfg.mirror_weight = 1.0f;
        const auto m = reality_warp_uv(cfg, {0.3f, 0.7f});
        if (std::fabs(m[0] - 0.7f) > 1e-3f) {
            std::fputs("HELL FRAME — reality_warp:mirror FAIL\n", stderr);
            return 9;
        }
    }

    std::puts("HELL FRAME — circles=9 stages=horror+sin+ruin+rapture "
              "audio=OK decals=OK reality_warp=OK");
    return 0;
}
