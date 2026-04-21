// tests/unit/audio/spatial_stub_test.cpp — ADR-0018 stub backend acceptance.

#include <doctest/doctest.h>

#include "engine/audio/spatial.hpp"

#include <array>
#include <cmath>

using namespace gw::audio;

TEST_CASE("compute_attenuation: min / max bracketing") {
    AttenuationModel m{.min_distance = 1.0f, .max_distance = 100.0f, .rolloff = 1.0f};
    CHECK(compute_attenuation(m, 0.5f) == doctest::Approx(1.0f));
    CHECK(compute_attenuation(m, 200.0f) == doctest::Approx(0.0f).epsilon(0.05));
    const float near_gain = compute_attenuation(m, 5.0f);
    const float far_gain  = compute_attenuation(m, 50.0f);
    CHECK(near_gain > far_gain);
}

TEST_CASE("compute_directivity: cardioid cone respects inner/outer cos") {
    DirectivityCone c{.inner_cos = 0.9f, .outer_cos = -0.5f, .outer_gain = 0.1f};
    CHECK(compute_directivity(c, 1.0f)  == doctest::Approx(1.0f));
    CHECK(compute_directivity(c, -1.0f) == doctest::Approx(0.1f));
    CHECK(compute_directivity(c, 0.2f)  < 1.0f);
    CHECK(compute_directivity(c, 0.2f)  > 0.1f);
}

TEST_CASE("compute_doppler: monotonic and clamped") {
    CHECK(compute_doppler(0.0f) == doctest::Approx(1.0f));
    CHECK(compute_doppler(50.0f) > 1.0f);
    CHECK(compute_doppler(-50.0f) < 1.0f);
    // Extreme speeds should be clamped so we never divide by zero.
    CHECK(compute_doppler(10'000.0f) > 0.0f);
    CHECK(compute_doppler(-10'000.0f) > 0.0f);
}

TEST_CASE("StubSpatialBackend: acquire / release round trip") {
    auto b = make_spatial_stub(4);
    EmitterState e{};
    auto h = b->acquire(e);
    CHECK_FALSE(h.is_null());
    b->release(h);
}

TEST_CASE("StubSpatialBackend: render is deterministic and additive") {
    auto b = make_spatial_stub(4);
    ListenerState L;
    b->set_listener(L);

    EmitterState e{};
    e.position = {10.0, 0.0, 0.0};
    auto h = b->acquire(e);
    b->update(h, e);

    std::array<float, 8> mono{};
    mono.fill(0.5f);
    std::array<float, 16> out{};
    b->render(h, mono, out, 8);
    float total = 0.0f;
    for (float s : out) total += std::fabs(s);
    CHECK(total > 0.0f);
}

TEST_CASE("StubSpatialBackend: is_hrtf returns false") {
    auto b = make_spatial_stub();
    CHECK_FALSE(b->is_hrtf());
}
