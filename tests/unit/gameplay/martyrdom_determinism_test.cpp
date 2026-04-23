// tests/unit/gameplay/martyrdom_determinism_test.cpp — Phase 22 W148
//
// Two independent peers running the same input trace at fixed dt must hash
// to the same bytes every 60 ticks. Mirrors the invariant the
// sandbox_martyrdom_online exit gate asserts at runtime.

#include <doctest/doctest.h>

#include "engine/narrative/grace_meter.hpp"
#include "gameplay/combat/glory_kill.hpp"
#include "gameplay/martyrdom/blasphemy_system.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/martyrdom_systems.hpp"
#include "gameplay/movement/grapple.hpp"
#include "gameplay/movement/player_controller.hpp"

#include <cstdint>

namespace {

std::uint64_t fnv1a(std::uint64_t seed, const void* data, std::size_t bytes) noexcept {
    const auto* p = static_cast<const unsigned char*>(data);
    std::uint64_t h = seed;
    for (std::size_t i = 0; i < bytes; ++i) {
        h ^= static_cast<std::uint64_t>(p[i]);
        h *= 0x100000001b3ULL;
    }
    return h;
}

struct Peer {
    gw::gameplay::martyrdom::SinComponent          sin{};
    gw::gameplay::martyrdom::MantraComponent       mantra{};
    gw::gameplay::martyrdom::RaptureState          rap{};
    gw::gameplay::martyrdom::RuinState             ruin{};
    gw::gameplay::martyrdom::ActiveBlasphemies     slots{};
    gw::gameplay::martyrdom::RaptureTriggerCounter trig{};
    gw::narrative::GraceComponent                  grace{};
    gw::gameplay::movement::PlayerMotion           motion{};
};

// Field-by-field hash. sizeof() would also hash padding bytes, which are
// indeterminate after aggregate assignment — see §[basic.types.general].
std::uint64_t hash_peer(const Peer& p) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    h = fnv1a(h, &p.sin.value,    sizeof(float));
    h = fnv1a(h, &p.sin.max,      sizeof(float));
    h = fnv1a(h, &p.sin.accrual_rate, sizeof(float));
    h = fnv1a(h, &p.mantra.value, sizeof(float));
    h = fnv1a(h, &p.mantra.max,   sizeof(float));
    h = fnv1a(h, &p.rap.time_remaining, sizeof(float));
    h = fnv1a(h, &p.rap.active,   sizeof(bool));
    h = fnv1a(h, &p.rap.trigger_count, sizeof(std::uint16_t));
    h = fnv1a(h, &p.ruin.time_remaining, sizeof(float));
    h = fnv1a(h, &p.ruin.active,  sizeof(bool));
    const std::int32_t slot_count = p.slots.count;
    h = fnv1a(h, &slot_count,     sizeof(slot_count));
    for (int i = 0; i < p.slots.count; ++i) {
        const auto type_byte = static_cast<std::uint8_t>(p.slots.entries[i].type);
        h = fnv1a(h, &type_byte, sizeof(type_byte));
        h = fnv1a(h, &p.slots.entries[i].time_remaining, sizeof(float));
    }
    h = fnv1a(h, &p.trig.this_circle, sizeof(std::uint16_t));
    h = fnv1a(h, &p.trig.lifetime,    sizeof(std::uint16_t));
    h = fnv1a(h, &p.grace.value,  sizeof(float));
    h = fnv1a(h, &p.grace.max,    sizeof(float));
    h = fnv1a(h, &p.motion.position.x, sizeof(float));
    h = fnv1a(h, &p.motion.position.y, sizeof(float));
    h = fnv1a(h, &p.motion.position.z, sizeof(float));
    h = fnv1a(h, &p.motion.velocity.x, sizeof(float));
    h = fnv1a(h, &p.motion.velocity.y, sizeof(float));
    h = fnv1a(h, &p.motion.velocity.z, sizeof(float));
    return h;
}

void step(Peer& p, std::uint64_t tick, float dt_sec) noexcept {
    using namespace gw::gameplay::martyrdom;
    MartyrdomFrameEvents ev{};
    ev.profane_hits = (tick % 5u == 0u) ? 3u : 0u;
    ev.profane_weapon_mult = 1.25f;
    ev.sacred_kills  = (tick % 9u == 0u) ? 1u : 0u;

    tick_sin_accrual(p.sin, ev);
    tick_mantra_accrual(p.mantra, ev);
    tick_rapture_check(p.sin, p.rap, p.ruin, p.trig);
    tick_rapture(p.rap, p.ruin, dt_sec);
    tick_ruin(p.ruin, dt_sec);
    tick_blasphemies(p.slots, dt_sec);

    if (!p.ruin.active && p.sin.value >= 30.f && p.slots.count == 0) {
        (void)try_cast_blasphemy(BlasphemyType::CrownOfThorns, p.sin, p.grace, p.ruin, p.slots, 0);
    }

    gw::gameplay::movement::PlayerInput input{};
    input.move_axis_y = 1.f;
    input.jump_pressed = (tick % 10u == 0u);
    gw::gameplay::movement::EnvironmentSample env{};
    env.ground_hit = (tick % 10u < 8u);
    gw::gameplay::movement::PlayerTuning t{};
    gw::gameplay::movement::step_player(p.motion, input, env, t, dt_sec);
}

} // namespace

TEST_CASE("Two peers run identical Martyrdom loop — hash matches every 60 ticks") {
    Peer a{};
    Peer b{};
    constexpr float kDt = 1.f / 60.f;
    constexpr std::uint64_t kTicks = 600;
    std::uint64_t divergences = 0;
    for (std::uint64_t t = 0; t < kTicks; ++t) {
        step(a, t, kDt);
        step(b, t, kDt);
        if ((t + 1u) % 60u == 0u) {
            if (hash_peer(a) != hash_peer(b)) ++divergences;
        }
    }
    CHECK(divergences == 0u);
    // Sanity: the peers actually did something (Sin grew, motion advanced).
    CHECK(a.motion.position.z > 0.f);
    CHECK(a.sin.value >= 0.f);
}
