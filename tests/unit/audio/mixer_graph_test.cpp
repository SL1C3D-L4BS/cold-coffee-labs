// tests/unit/audio/mixer_graph_test.cpp — ADR-0017 §2.2 canonical topology.

#include <doctest/doctest.h>

#include "engine/audio/mixer_graph.hpp"

using namespace gw::audio;

TEST_CASE("MixerGraph: canonical topology") {
    MixerGraph g;
    CHECK(g.topology_matches_canonical());
}

TEST_CASE("MixerGraph: gain walks bus hierarchy") {
    MixerGraph g;
    g.set_bus_volume(BusId::Master, 0.5f);
    g.set_bus_volume(BusId::SFX,    0.5f);
    g.set_bus_volume(BusId::SFX_Weapons, 0.5f);
    const float gain = g.compute_effective_gain(BusId::SFX_Weapons);
    CHECK(gain == doctest::Approx(0.125f));
}

TEST_CASE("MixerGraph: mute short-circuits gain") {
    MixerGraph g;
    g.set_bus_mute(BusId::SFX, true);
    CHECK(g.compute_effective_gain(BusId::SFX_Weapons) == 0.0f);
    CHECK(g.compute_effective_gain(BusId::Music_Combat) != 0.0f);
}

TEST_CASE("MixerGraph: duck_db attenuates") {
    MixerGraph g;
    // Force a clean baseline — canonical defaults have Music at 0.8.
    g.set_bus_volume(BusId::Music, 1.0f);
    g.apply_duck_db(BusId::Music, -6.0f);  // halves the gain
    const float gain = g.compute_effective_gain(BusId::Music_Combat);
    CHECK(gain == doctest::Approx(0.5f).epsilon(0.02));
    g.reset_ducking();
    CHECK(g.compute_effective_gain(BusId::Music_Combat) == doctest::Approx(1.0f));
}

TEST_CASE("MixerGraph: vacuum mutes SFX but leaves music") {
    MixerGraph g;
    g.set_vacuum(true);
    CHECK(g.compute_effective_gain(BusId::SFX_Environment) == 0.0f);
    CHECK(g.compute_effective_gain(BusId::Music_Space) != 0.0f);
}

TEST_CASE("MixerGraph: downmix mode setter") {
    MixerGraph g;
    CHECK(g.downmix_mode() == DownmixMode::Stereo);
    g.set_downmix_mode(DownmixMode::Mono);
    CHECK(g.downmix_mode() == DownmixMode::Mono);
}
