// tests/unit/input/device_test.cpp — ADR-0020 device registry + processor.

#include <doctest/doctest.h>

#include "engine/input/device.hpp"

using namespace gw::input;
namespace math = gw::math;

TEST_CASE("DeviceId: make/encode/decode is stable") {
    auto id = DeviceId::make(0xDEADBEEFu, /*player*/1, /*flags*/Cap_Gyro);
    CHECK(id.guid_hash() == 0xDEADBEEFu);
    CHECK(id.player_index() == 1);
    CHECK(id.flags() == Cap_Gyro);
    CHECK_FALSE(id.is_null());
}

TEST_CASE("DeviceRegistry: add / remove / find") {
    DeviceRegistry r;
    Device d{};
    d.id = DeviceId::make(1, 0, 0);
    d.kind = DeviceKind::Gamepad;
    d.caps = Cap_Buttons | Cap_Axes;
    CHECK(r.on_connected(d));
    CHECK(r.size() == 1);
    CHECK(r.find(d.id) != nullptr);
    CHECK(r.on_removed(d.id));
    CHECK(r.size() == 0);
    CHECK(r.find(d.id) == nullptr);
}

TEST_CASE("DeviceRegistry: detect_adaptive_flags recognizes XAC and Quadstick") {
    auto xac = DeviceRegistry::detect_adaptive_flags("Microsoft Xbox Adaptive", 0x045E, 0x0B0A);
    CHECK((xac & Cap_Adaptive) != 0);

    auto qs = DeviceRegistry::detect_adaptive_flags("Quadstick FPS", 0x0000, 0x0000);
    CHECK((qs & Cap_Quadstick) != 0);
    CHECK((qs & Cap_Adaptive) != 0);

    auto none = DeviceRegistry::detect_adaptive_flags("Xbox One Controller", 0x045E, 0x02FD);
    CHECK(none == 0u);
}

TEST_CASE("process_scalar: deadzone zeroes near-centre, clamps extremes") {
    Processor p{};
    p.dz.inner = 0.1f;
    p.dz.outer = 1.0f;
    float state = 0.0f;
    CHECK(process_scalar(p, 0.05f, state) == 0.0f);
    CHECK(process_scalar(p, 1.5f, state) == doctest::Approx(1.0f));
    CHECK(process_scalar(p, -1.5f, state) == doctest::Approx(-1.0f));
}

TEST_CASE("process_scalar: invert flips sign") {
    Processor p{};
    p.dz.inner = 0.0f;   // disable default 0.15 deadzone for a clean sign test
    p.invert = true;
    float state = 0.0f;
    CHECK(process_scalar(p, 0.5f, state) == doctest::Approx(-0.5f));
}

TEST_CASE("process_stick: radial deadzone") {
    Processor p{};
    p.dz.inner = 0.2f;
    p.dz.outer = 1.0f;
    p.dz.radial = true;
    math::Vec2f state{};
    auto out = process_stick(p, math::Vec2f(0.1f, 0.1f), state);
    CHECK(out.x() == doctest::Approx(0.0f));
    CHECK(out.y() == doctest::Approx(0.0f));
    auto big = process_stick(p, math::Vec2f(1.0f, 0.0f), state);
    CHECK(big.x() > 0.9f);
}
