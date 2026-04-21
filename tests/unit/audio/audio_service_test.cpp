// tests/unit/audio/audio_service_test.cpp — ADR-0017 façade integration.

#include <doctest/doctest.h>

#include "engine/audio/audio_backend.hpp"
#include "engine/audio/audio_service.hpp"
#include "engine/audio/spatial.hpp"

using namespace gw::audio;

namespace {

AudioConfig test_cfg() {
    AudioConfig c;
    c.sample_rate = 48'000;
    c.frames_period = 128;
    c.channels = 2;
    c.voices_3d = 4;
    c.voices_2d = 4;
    c.deterministic = true;
    c.enable_hrtf = false;
    return c;
}

} // namespace

TEST_CASE("AudioService: constructs and destructs cleanly") {
    AudioService svc(test_cfg(), make_null_backend(), make_spatial_stub());
    CHECK(svc.stats().active_voices_2d == 0);
    CHECK(svc.stats().active_voices_3d == 0);
}

TEST_CASE("AudioService: play_2d yields live voice") {
    AudioService svc(test_cfg(), make_null_backend(), make_spatial_stub());
    Play2D p{.bus = BusId::SFX_UI};
    auto h = svc.play_2d(AudioClipId{1}, p);
    CHECK_FALSE(h.is_null());
    svc.stop(h);
}

TEST_CASE("AudioService: bus volume is forwarded to the mixer") {
    AudioService svc(test_cfg(), make_null_backend(), make_spatial_stub());
    svc.set_bus_volume(BusId::SFX, 0.5f);
    CHECK(svc.mixer().bus_volume(BusId::SFX) == doctest::Approx(0.5f));
}

TEST_CASE("AudioService: vacuum flag toggles mixer state") {
    AudioService svc(test_cfg(), make_null_backend(), make_spatial_stub());
    svc.set_vacuum(true);
    CHECK(svc.mixer().in_vacuum());
    svc.set_vacuum(false);
    CHECK_FALSE(svc.mixer().in_vacuum());
}

TEST_CASE("AudioService: pump_frames drives null backend without errors") {
    AudioService svc(test_cfg(), make_null_backend(), make_spatial_stub());
    Play2D p{.bus = BusId::SFX_UI};
    auto h = svc.play_2d(AudioClipId{1}, p);
    (void)h;
    svc.pump_frames(256);
    // A tick of the main-thread update() bumps frame_counter_ → frames_rendered.
    svc.update(1.0 / 60.0, ListenerState{});
    CHECK(svc.stats().frames_rendered == 1);
}
