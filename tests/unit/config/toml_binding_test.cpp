// tests/unit/config/toml_binding_test.cpp — Phase 11 Wave 11B (ADR-0024).

#include <doctest/doctest.h>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/toml_binding.hpp"

using namespace gw::config;

TEST_CASE("toml: load_domain_toml parses bool/int/float/string under a table") {
    CVarRegistry r;
    auto b = r.register_bool({"audio.enabled", false, kCVarPersist, {}, {}, false, ""});
    auto i = r.register_i32 ({"audio.period",  512,  kCVarPersist, 0, 0,  false, ""});
    auto f = r.register_f32 ({"audio.volume",  0.0f, kCVarPersist, 0.0f, 1.0f, true, ""});
    auto s = r.register_string({"audio.name",  std::string{"old"}, kCVarPersist, {}, {}, false, ""});

    constexpr std::string_view src =
        "# a comment\n"
        "[audio]\n"
        "enabled = true\n"
        "period  = 256\n"
        "volume  = 0.75\n"
        "name    = \"retro\"\n";

    auto rc = load_domain_toml(src, "audio.", r);
    CHECK(rc.has_value());
    CHECK(*r.get_bool(b.id) == true);
    CHECK(*r.get_i32(i.id)  == 256);
    CHECK(*r.get_f32(f.id)  == doctest::Approx(0.75f));
    CHECK(*r.get_string(s.id) == "retro");
}

TEST_CASE("toml: ignores out-of-prefix keys silently") {
    CVarRegistry r;
    auto b = r.register_bool({"audio.enabled", false, kCVarPersist, {}, {}, false, ""});
    constexpr std::string_view src =
        "[input]\n"
        "invert_y = true\n"
        "[audio]\n"
        "enabled = true\n";
    auto rc = load_domain_toml(src, "audio.", r);
    CHECK(rc.has_value());
    CHECK(*r.get_bool(b.id) == true);
}

TEST_CASE("toml: unterminated table header is a hard error") {
    CVarRegistry r;
    auto rc = load_domain_toml("[bad\nkey=1\n", "foo.", r);
    CHECK_FALSE(rc.has_value());
}

TEST_CASE("toml: save_domain_toml emits persisted keys sorted") {
    CVarRegistry r;
    (void)r.register_bool({"audio.zzz", true,  kCVarPersist, {}, {}, false, ""});
    (void)r.register_i32 ({"audio.aaa", 5,     kCVarPersist, 0, 0,  false, ""});
    (void)r.register_bool({"audio.skip", true, kCVarNone,    {}, {}, false, ""});  // not persisted
    auto text = save_domain_toml("audio.", r);
    CHECK(text.find("aaa") != std::string::npos);
    CHECK(text.find("zzz") != std::string::npos);
    CHECK(text.find("skip") == std::string::npos);
    // aaa should appear before zzz.
    CHECK(text.find("aaa") < text.find("zzz"));
}
