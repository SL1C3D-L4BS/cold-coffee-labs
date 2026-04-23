// tests/unit/audio/martyrdom_audio_test.cpp — Phase 21 W139 (ADR-0108).

#include "engine/audio/martyrdom_audio_controller.hpp"

#include <doctest/doctest.h>

#include <cstring>

using gw::audio::DialogueTrigger;
using gw::audio::MartyrdomAudioInput;
using gw::audio::MartyrdomAudioState;
using gw::audio::step_martyrdom_audio;
using gw::audio::martyrdom_bus_names;

TEST_CASE("martyrdom_audio: sin_choir gain rises with sin_current") {
    MartyrdomAudioState s{};
    MartyrdomAudioInput in{};
    in.dt_sec           = 1.0f / 60.0f;
    in.sin_current      = 0.0f;
    in.prev_sin_current = 0.0f;

    for (int i = 0; i < 120; ++i) {
        in.prev_sin_current = in.sin_current;
        in.sin_current      = 80.0f;
        const auto out      = step_martyrdom_audio(s, in);
        s                   = out.state;
    }
    CHECK(s.sin_choir_gain > 0.5f);
}

TEST_CASE("martyrdom_audio: rapture onset emits a dialogue trigger once") {
    MartyrdomAudioState s{};
    MartyrdomAudioInput in{};
    in.dt_sec             = 1.0f / 60.0f;
    in.prev_rapture_active = false;
    in.rapture_active      = true;

    const auto first  = step_martyrdom_audio(s, in);
    CHECK(first.trigger == DialogueTrigger::RaptureOnset);

    // The next tick should NOT re-fire the trigger while the state stays on.
    in.prev_rapture_active = true;
    const auto second = step_martyrdom_audio(first.state, in);
    CHECK(second.trigger != DialogueTrigger::RaptureOnset);
}

TEST_CASE("martyrdom_audio: sin thresholds fire exactly once when crossing up") {
    MartyrdomAudioState s{};
    MartyrdomAudioInput in{};
    in.dt_sec = 1.0f / 60.0f;

    in.prev_sin_current = 29.0f;
    in.sin_current      = 31.0f;
    const auto t30 = step_martyrdom_audio(s, in);
    CHECK(t30.trigger == DialogueTrigger::SinThreshold30);

    in.prev_sin_current = 31.0f;
    in.sin_current      = 32.0f;
    const auto t30b = step_martyrdom_audio(t30.state, in);
    CHECK(t30b.trigger != DialogueTrigger::SinThreshold30);

    in.prev_sin_current = 59.5f;
    in.sin_current      = 60.5f;
    const auto t60 = step_martyrdom_audio(t30b.state, in);
    CHECK(t60.trigger == DialogueTrigger::SinThreshold60);
}

TEST_CASE("martyrdom_audio: bus names are stable") {
    const auto names = martyrdom_bus_names();
    CHECK(std::strcmp(names.sin_choir, "audio.martyrdom.sin_choir") == 0);
    CHECK(std::strcmp(names.rapture_thunder, "audio.martyrdom.rapture_thunder") == 0);
    CHECK(std::strcmp(names.ruin_drone, "audio.martyrdom.ruin_drone") == 0);
}
