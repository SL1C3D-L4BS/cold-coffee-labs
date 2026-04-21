// tests/unit/audio/atmosphere_filter_test.cpp — ADR-0018 atmospheric LPF.

#include <doctest/doctest.h>

#include "engine/audio/atmosphere_filter.hpp"

using namespace gw::audio;

TEST_CASE("atmosphere_cutoff_hz: vacuum keeps filter wide open") {
    AtmosphereFilterConfig cfg;
    const float fc = atmosphere_cutoff_hz(cfg, /*density*/0.0f, /*dist*/50.0f);
    CHECK(fc == doctest::Approx(cfg.base_cutoff_hz));
}

TEST_CASE("atmosphere_cutoff_hz: dense air attenuates high freq with distance") {
    AtmosphereFilterConfig cfg;
    const float near_fc = atmosphere_cutoff_hz(cfg, 1.0f, 1.0f);
    const float far_fc  = atmosphere_cutoff_hz(cfg, 1.0f, 200.0f);
    CHECK(near_fc > far_fc);
    CHECK(far_fc >= cfg.min_cutoff_hz);
}

TEST_CASE("BiquadLPF: pass-through at nyquist") {
    BiquadLPF f;
    f.design(48'000.0f, 24'000.0f);
    float buf[4] = {1.0f, -1.0f, 0.5f, -0.5f};
    f.process_stereo(buf, 2);
    CHECK(buf[0] == doctest::Approx(1.0f));
    CHECK(buf[2] == doctest::Approx(0.5f));
}

TEST_CASE("BiquadLPF: state is silenced by reset()") {
    BiquadLPF f;
    f.design(48'000.0f, 500.0f);
    float buf[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    f.process_stereo(buf, 2);
    f.reset();
    CHECK(f.z1_l == 0.0f);
    CHECK(f.z2_l == 0.0f);
    CHECK(f.z1_r == 0.0f);
    CHECK(f.z2_r == 0.0f);
}

TEST_CASE("BiquadLPF: design is stable at typical cutoffs") {
    BiquadLPF f;
    f.design(48'000.0f, 1'000.0f);
    float sum = 0.0f;
    float buf[16]{};
    for (int i = 0; i < 8; ++i) buf[2 * i] = 1.0f;
    f.process_stereo(buf, 8);
    for (int i = 0; i < 8; ++i) sum += buf[2 * i];
    CHECK(sum > 0.0f);
    CHECK(sum < 10.0f);  // bounded
}
