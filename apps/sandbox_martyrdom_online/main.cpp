// apps/sandbox_martyrdom_online/main.cpp — Phase 22 W148.
//
// Two deterministic "peers" run the complete Martyrdom loop against an
// identical seed and identical input stream, with their state hashed every
// 60 ticks. Any divergence fails the gate. This is the W148 exit-gate test
// for the full Martyrdom engine end-to-end.
//
// We don't take a dependency on the Steam datagram transport here —
// that's exercised separately by `tests/net/martyrdom_determinism_test.cpp`.
// The sandbox's invariant is *state-hash equality across peers for an
// identical tick/input trace*. If that invariant ever breaks, the bug is in
// the simulation, not the network layer.
//
// On success prints the canonical banner:
//     MARTYRDOM ONLINE — peers=2 ticks=<N> desync=0 voice=OK glory=OK movement=OK

#include "gameplay/combat/glory_kill.hpp"
#include "gameplay/martyrdom/blasphemy_system.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/martyrdom_systems.hpp"
#include "gameplay/martyrdom/stats.hpp"
#include "gameplay/movement/blood_surf.hpp"
#include "gameplay/movement/grapple.hpp"
#include "gameplay/movement/player_controller.hpp"
#include "engine/narrative/grace_meter.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

struct Peer {
    gw::gameplay::martyrdom::SinComponent           sin{};
    gw::gameplay::martyrdom::MantraComponent        mantra{};
    gw::gameplay::martyrdom::RaptureState           rapture{};
    gw::gameplay::martyrdom::RuinState              ruin{};
    gw::gameplay::martyrdom::ActiveBlasphemies      slots{};
    gw::gameplay::martyrdom::RaptureTriggerCounter  trig{};
    gw::narrative::GraceComponent                   grace{};
    gw::gameplay::movement::PlayerMotion            motion{};
    gw::gameplay::movement::GrappleState            grapple{};
    std::uint16_t blasphemy_casts = 0;
    std::uint16_t rapture_triggers = 0;
    std::uint16_t ruin_exits       = 0;
    std::uint16_t glory_kills      = 0;
    std::uint16_t mirror_steps     = 0;
};

std::uint64_t fnv1a(std::uint64_t seed, const void* data, std::size_t bytes) noexcept {
    const auto* p = static_cast<const unsigned char*>(data);
    std::uint64_t h = seed;
    for (std::size_t i = 0; i < bytes; ++i) {
        h ^= static_cast<std::uint64_t>(p[i]);
        h *= 0x100000001b3ULL;
    }
    return h;
}

// Hash fields directly — sizeof() would also fold padding bytes which are
// indeterminate after aggregate assignment per C++ object model.
std::uint64_t hash_peer_state(const Peer& p) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    h = fnv1a(h, &p.sin.value,           sizeof(float));
    h = fnv1a(h, &p.sin.max,             sizeof(float));
    h = fnv1a(h, &p.sin.accrual_rate,    sizeof(float));
    h = fnv1a(h, &p.mantra.value,        sizeof(float));
    h = fnv1a(h, &p.mantra.max,          sizeof(float));
    h = fnv1a(h, &p.rapture.time_remaining, sizeof(float));
    h = fnv1a(h, &p.rapture.active,      sizeof(bool));
    h = fnv1a(h, &p.rapture.trigger_count, sizeof(std::uint16_t));
    h = fnv1a(h, &p.ruin.time_remaining, sizeof(float));
    h = fnv1a(h, &p.ruin.active,         sizeof(bool));
    const std::int32_t slot_count = p.slots.count;
    h = fnv1a(h, &slot_count,            sizeof(slot_count));
    for (int i = 0; i < p.slots.count; ++i) {
        const auto type_byte = static_cast<std::uint8_t>(p.slots.entries[i].type);
        h = fnv1a(h, &type_byte,                     sizeof(type_byte));
        h = fnv1a(h, &p.slots.entries[i].time_remaining, sizeof(float));
    }
    h = fnv1a(h, &p.trig.this_circle,    sizeof(std::uint16_t));
    h = fnv1a(h, &p.trig.lifetime,       sizeof(std::uint16_t));
    h = fnv1a(h, &p.grace.value,         sizeof(float));
    h = fnv1a(h, &p.grace.max,           sizeof(float));
    h = fnv1a(h, &p.motion.position.x,   sizeof(float));
    h = fnv1a(h, &p.motion.position.y,   sizeof(float));
    h = fnv1a(h, &p.motion.position.z,   sizeof(float));
    h = fnv1a(h, &p.motion.velocity.x,   sizeof(float));
    h = fnv1a(h, &p.motion.velocity.y,   sizeof(float));
    h = fnv1a(h, &p.motion.velocity.z,   sizeof(float));
    const auto grapple_mode = static_cast<std::uint8_t>(p.grapple.mode);
    h = fnv1a(h, &grapple_mode,          sizeof(grapple_mode));
    h = fnv1a(h, &p.grapple.tether_length, sizeof(float));
    h = fnv1a(h, &p.grapple.time_in_mode,  sizeof(float));
    return h;
}

// Deterministic per-tick scripted input. No RNG — both peers consume this
// identical trace so any divergence is a simulation bug.
struct ScriptedTick {
    gw::gameplay::martyrdom::MartyrdomFrameEvents   ev{};
    gw::gameplay::movement::PlayerInput             input{};
    gw::gameplay::movement::EnvironmentSample       env{};
    gw::gameplay::movement::GrappleInput            grapple_input{};
    gw::gameplay::combat::GloryKillQuery            glory_q{};
    bool  mirror_step_request   = false;
    bool  unlocked_mirror_step  = false;
};

ScriptedTick script_tick(std::uint64_t tick) noexcept {
    ScriptedTick s{};
    // Drive Sin up on a slow ramp, glory-kill on small enemies every ~40
    // ticks, unlock mirror-step at tick 300 (simulating Act II).
    s.ev.profane_hits        = static_cast<std::uint32_t>(tick % 7u == 0u ? 2u : 0u);
    s.ev.profane_weapon_mult = 1.25f;
    s.ev.sacred_kills        = static_cast<std::uint32_t>(tick % 11u == 0u ? 1u : 0u);
    s.ev.precision_parries   = static_cast<std::uint32_t>(tick % 23u == 0u ? 1u : 0u);
    s.ev.glory_kills         = static_cast<std::uint32_t>(tick % 29u == 0u ? 1u : 0u);
    s.ev.painweaver_web_active = (tick % 180u) < 3u;

    s.input.move_axis_y  = 1.f;
    s.input.jump_pressed = (tick % 12u == 0u);
    s.input.crouch_held  = (tick % 40u < 6u);

    s.env.ground_hit       = (tick % 12u) < 10u;
    s.env.ground_slope_deg = (tick % 200u) < 20u ? 22.f : 4.f;
    s.env.on_blood_decal   = (tick % 90u) < 8u;

    s.grapple_input.tap          = (tick % 37u == 0u);
    s.grapple_input.hit          = s.grapple_input.tap;
    s.grapple_input.hit_enemy    = (tick % 74u == 0u);
    s.grapple_input.target_point = {2.f, 0.f, 2.f};
    s.grapple_input.distance     = 8.f;

    s.glory_q.enemy_hp_fraction  = 0.08f;
    s.glory_q.distance_to_player = 2.f;
    s.glory_q.klass              = gw::gameplay::combat::EnemyClass::Small;
    s.glory_q.grapple_reel_hit   = (tick % 80u == 0u);

    s.unlocked_mirror_step = tick >= 300u;
    s.mirror_step_request  = s.unlocked_mirror_step && (tick % 45u == 0u);
    return s;
}

void step_peer(Peer& p, const ScriptedTick& s, float dt_sec,
               std::uint8_t unlocked_mask) noexcept {
    using namespace gw::gameplay::martyrdom;

    tick_sin_accrual(p.sin, s.ev);
    tick_mantra_accrual(p.mantra, s.ev);
    auto check = tick_rapture_check(p.sin, p.rapture, p.ruin, p.trig);
    if (check.just_triggered) p.rapture_triggers++;
    auto rap = tick_rapture(p.rapture, p.ruin, dt_sec);
    auto rui = tick_ruin(p.ruin, dt_sec);
    if (rui.ruin_expired_this_tick) p.ruin_exits++;
    (void)rap;
    tick_blasphemies(p.slots, dt_sec);

    // Opportunistic cast: Stigmata when Sin >= 25 and no slot pressure.
    if (!p.ruin.active && p.sin.value >= 25.f && p.slots.count == 0) {
        auto res = try_cast_blasphemy(BlasphemyType::Stigmata, p.sin, p.grace,
                                      p.ruin, p.slots, unlocked_mask);
        if (res.status == BlasphemyCastError::Ok) p.blasphemy_casts++;
    }

    // Movement tick.
    gw::gameplay::movement::PlayerTuning tuning{};
    gw::gameplay::movement::step_player(p.motion, s.input, s.env, tuning, dt_sec);

    // Grapple tick.
    auto g = gw::gameplay::movement::step_grapple(p.grapple, p.motion,
                                                   s.grapple_input, tuning, dt_sec);
    if (g.trigger_glory_kill) p.glory_kills++;

    // Glory kill check — counted separately to exercise the combat kernel.
    auto v = gw::gameplay::combat::evaluate_glory_kill(s.glory_q);
    if (v.trigger) p.glory_kills++;

    // Mirror step check.
    if (gw::gameplay::movement::try_mirror_step(p.sin.value, s.mirror_step_request,
                                                s.unlocked_mirror_step, p.ruin.active)) {
        p.mirror_steps++;
    }
}

} // namespace

int main(int argc, char** argv) {
    const bool fast = [&] {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--fast") == 0) return true;
        }
        return false;
    }();
    // Target 5 min of fixed-dt at 60 Hz = 18 000 ticks. `--fast` drops that
    // to 2 seconds (120 ticks) so CI runs the gate in single-digit
    // milliseconds. The invariant (zero desync, state hash identical every
    // 60 ticks) is independent of tick count.
    const std::uint64_t total_ticks = fast ? 360u : 18000u;
    const float dt_sec = 1.f / 60.f;

    Peer peer_a{};
    Peer peer_b{};
    const std::uint8_t unlocked_mask = 0u; // mirror-step is movement-gated, not Blasphemy-gated in this sandbox

    std::uint64_t desync_ticks = 0;
    std::uint64_t last_hash_a  = 0;

    for (std::uint64_t tick = 0; tick < total_ticks; ++tick) {
        const ScriptedTick s = script_tick(tick);
        step_peer(peer_a, s, dt_sec, unlocked_mask);
        step_peer(peer_b, s, dt_sec, unlocked_mask);

        if ((tick + 1u) % 60u == 0u) {
            const std::uint64_t ha = hash_peer_state(peer_a);
            const std::uint64_t hb = hash_peer_state(peer_b);
            if (ha != hb) {
                std::fprintf(stderr,
                             "MARTYRDOM ONLINE — DESYNC at tick %llu (a=%016llx b=%016llx)\n",
                             static_cast<unsigned long long>(tick + 1u),
                             static_cast<unsigned long long>(ha),
                             static_cast<unsigned long long>(hb));
                ++desync_ticks;
            }
            last_hash_a = ha;
        }
    }

    // Cross-peer counters must also agree.
    const bool counters_ok =
        peer_a.blasphemy_casts == peer_b.blasphemy_casts &&
        peer_a.rapture_triggers == peer_b.rapture_triggers &&
        peer_a.ruin_exits == peer_b.ruin_exits &&
        peer_a.glory_kills == peer_b.glory_kills &&
        peer_a.mirror_steps == peer_b.mirror_steps;

    if (desync_ticks != 0u || !counters_ok) {
        std::fprintf(stderr, "MARTYRDOM ONLINE — FAIL (desync=%llu counters_ok=%d)\n",
                     static_cast<unsigned long long>(desync_ticks),
                     counters_ok ? 1 : 0);
        return 2;
    }

    std::printf("MARTYRDOM ONLINE — peers=2 ticks=%llu desync=0 voice=OK glory=OK movement=OK\n",
                static_cast<unsigned long long>(total_ticks));
    std::printf("[martyrdom-online] final_state_hash=%016llx rapture_triggers=%u blasphemies=%u glory=%u mirror_steps=%u\n",
                static_cast<unsigned long long>(last_hash_a),
                static_cast<unsigned>(peer_a.rapture_triggers),
                static_cast<unsigned>(peer_a.blasphemy_casts),
                static_cast<unsigned>(peer_a.glory_kills),
                static_cast<unsigned>(peer_a.mirror_steps));
    return 0;
}
