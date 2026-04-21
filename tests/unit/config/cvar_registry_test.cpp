// tests/unit/config/cvar_registry_test.cpp — Phase 11 Wave 11B (ADR-0024).

#include <doctest/doctest.h>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/standard_cvars.hpp"
#include "engine/core/events/event_bus.hpp"

using namespace gw::config;

TEST_CASE("CVarRegistry: register_bool returns a valid ref") {
    CVarRegistry r;
    auto h = r.register_bool({"test.bool", true, kCVarNone, {}, {}, false, "x"});
    CHECK(h.valid());
    CHECK(r.count() == 1);
    CHECK(r.get_bool(h.id).value_or(false) == true);
}

TEST_CASE("CVarRegistry: find returns invalid for missing names") {
    CVarRegistry r;
    CHECK_FALSE(r.find("does.not.exist").valid());
}

TEST_CASE("CVarRegistry: get/set round-trip for every scalar type") {
    CVarRegistry r;
    auto b = r.register_bool({"t.b", false, kCVarNone, {}, {}, false, ""});
    auto i = r.register_i32 ({"t.i", 0, kCVarNone, 0, 100, true, ""});
    auto j = r.register_i64 ({"t.j", 0, kCVarNone, 0, 0, false, ""});
    auto f = r.register_f32 ({"t.f", 0.0f, kCVarNone, 0.0f, 1.0f, true, ""});
    auto d = r.register_f64 ({"t.d", 0.0, kCVarNone, 0.0, 0.0, false, ""});
    auto s = r.register_string({"t.s", std::string{"x"}, kCVarNone, {}, {}, false, ""});

    CHECK(r.set_bool(b.id, true));
    CHECK(r.set_i32 (i.id, 42));
    CHECK(r.set_i64 (j.id, 123456789));
    CHECK(r.set_f32 (f.id, 0.5f));
    CHECK(r.set_f64 (d.id, 3.14));
    CHECK(r.set_string(s.id, "hello"));

    CHECK(*r.get_bool(b.id) == true);
    CHECK(*r.get_i32 (i.id) == 42);
    CHECK(*r.get_i64 (j.id) == 123456789);
    CHECK(*r.get_f32 (f.id) == doctest::Approx(0.5f));
    CHECK(*r.get_f64 (d.id) == doctest::Approx(3.14));
    CHECK(*r.get_string(s.id) == "hello");
}

TEST_CASE("CVarRegistry: range clamp is applied on numeric sets") {
    CVarRegistry r;
    auto i = r.register_i32({"t.ranged", 0, kCVarNone, 0, 10, true, ""});
    CHECK(r.set_i32(i.id, 99));
    CHECK(*r.get_i32(i.id) == 10);
    CHECK(r.set_i32(i.id, -99));
    CHECK(*r.get_i32(i.id) == 0);
}

TEST_CASE("CVarRegistry: read-only denies non-programmatic writes") {
    CVarRegistry r;
    auto b = r.register_bool({"t.ro", false, kCVarReadOnly, {}, {}, false, ""});
    CHECK_FALSE(r.set_bool(b.id, true, kOriginConsole));
    CHECK(*r.get_bool(b.id) == false);
    // Programmatic still writes through.
    CHECK(r.set_bool(b.id, true, kOriginProgrammatic));
    CHECK(*r.get_bool(b.id) == true);
}

TEST_CASE("CVarRegistry: set_from_string parses bool, int, float") {
    CVarRegistry r;
    (void)r.register_bool({"t.b", false, kCVarNone, {}, {}, false, ""});
    (void)r.register_i32 ({"t.i", 0, kCVarNone, 0, 0, false, ""});
    (void)r.register_f64 ({"t.d", 0.0, kCVarNone, 0, 0, false, ""});

    using R = CVarRegistry::SetResult;
    CHECK(r.set_from_string("t.b", "true")  == R::Ok);
    CHECK(r.set_from_string("t.b", "false") == R::Ok);
    CHECK(r.set_from_string("t.b", "wat")   == R::ParseError);
    CHECK(r.set_from_string("t.i", "42")    == R::Ok);
    CHECK(r.set_from_string("t.i", "nope")  == R::ParseError);
    CHECK(r.set_from_string("t.d", "3.14")  == R::Ok);
    CHECK(r.get_bool_or("t.b", true)  == false);
    CHECK(r.get_i32_or("t.i", 0)      == 42);
    CHECK(r.get_f64_or("t.d", 0.0)    == doctest::Approx(3.14));
}

TEST_CASE("CVarRegistry: enum labels + numeric fallback") {
    CVarRegistry r;
    const EnumVariant variants[] = {{"stereo", 0}, {"mono", 1}};
    auto e = r.register_enum("t.e", std::span<const EnumVariant>{variants, 2}, 0);
    CHECK(r.set_from_string("t.e", "mono") == CVarRegistry::SetResult::Ok);
    CHECK(*r.get_i32(e.id) == 1);
    CHECK(r.set_from_string("t.e", "0") == CVarRegistry::SetResult::Ok);
    CHECK(*r.get_i32(e.id) == 0);
}

TEST_CASE("CVarRegistry: ConfigCVarChanged fires on every write") {
    CVarRegistry r;
    gw::events::EventBus<gw::events::ConfigCVarChanged> bus;
    r.set_bus(&bus);
    int hits = 0;
    (void)bus.subscribe([&](const gw::events::ConfigCVarChanged&) { ++hits; });
    auto b = r.register_bool({"t.b", false, kCVarNone, {}, {}, false, ""});
    CHECK(r.set_bool(b.id, true));
    CHECK(r.set_bool(b.id, false));
    CHECK(hits == 2);
}

TEST_CASE("CVarRegistry: version bumps monotonically on writes") {
    CVarRegistry r;
    auto b = r.register_bool({"t.b", false, kCVarNone, {}, {}, false, ""});
    const auto v0 = r.version();
    CHECK(r.set_bool(b.id, true));
    CHECK(r.version() > v0);
}

TEST_CASE("CVarRegistry: reset_to_default restores default value") {
    CVarRegistry r;
    auto i = r.register_i32({"t.i", 5, kCVarNone, 0, 10, true, ""});
    CHECK(r.set_i32(i.id, 9));
    r.reset_to_default(i.id);
    CHECK(*r.get_i32(i.id) == 5);
}

TEST_CASE("CVarRegistry: cheats_tripped only when cheat CVar deviates") {
    CVarRegistry r;
    auto b = r.register_bool({"cheat.god", false, kCVarCheat, {}, {}, false, ""});
    CHECK_FALSE(r.cheats_tripped());
    CHECK(r.set_bool(b.id, true));
    CHECK(r.cheats_tripped());
    r.reset_to_default(b.id);
    CHECK_FALSE(r.cheats_tripped());
}

TEST_CASE("StandardCVars: registers every expected domain key") {
    CVarRegistry r;
    auto s = register_standard_cvars(r);
    (void)s;
    // Spot-check a representative CVar from each domain.
    CHECK(r.find("audio.master.volume").valid());
    CHECK(r.find("gfx.swapchain.vsync").valid());
    CHECK(r.find("input.look.invert_y").valid());
    CHECK(r.find("ui.text.scale").valid());
    CHECK(r.find("dev.console.allow_release").valid());
    CHECK(r.find("rng.seed").valid());
}
