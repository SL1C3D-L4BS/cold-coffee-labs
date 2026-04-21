// tests/unit/ui/settings_binder_test.cpp — Phase 11 Wave 11E (ADR-0027).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/ui/settings_binder.hpp"

using namespace gw::ui;
using namespace gw::config;

TEST_CASE("SettingsBinder: push_from_ui writes CVar via origin rml-binding") {
    CVarRegistry r;
    (void)r.register_f32({"audio.master.volume", 1.0f, kCVarPersist, 0.0f, 1.0f, true, ""});
    SettingsBinder b(r);
    b.bind({"slider-master", "audio.master.volume", 0});
    CHECK(b.push_from_ui("slider-master", "0.25"));
    CHECK(*r.get_f32(r.find("audio.master.volume")) == doctest::Approx(0.25f));
}

TEST_CASE("SettingsBinder: pull_for_ui stringifies CVar") {
    CVarRegistry r;
    auto id = r.register_bool({"ui.contrast.boost", true, kCVarPersist, {}, {}, false, ""});
    (void)id;
    SettingsBinder b(r);
    b.bind({"toggle-contrast", "ui.contrast.boost", 0});
    CHECK(b.pull_for_ui("toggle-contrast") == "true");
}

TEST_CASE("SettingsBinder: bind overwrites existing binding for the same element") {
    CVarRegistry r;
    (void)r.register_i32({"a.i", 1, kCVarPersist, 0, 0, false, ""});
    (void)r.register_i32({"b.i", 2, kCVarPersist, 0, 0, false, ""});
    SettingsBinder b(r);
    b.bind({"el", "a.i", 0});
    b.bind({"el", "b.i", 0});
    CHECK(b.cvar_for("el") == "b.i");
    CHECK(b.binding_count() == 1);
}
