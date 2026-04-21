// tests/unit/ui/ui_service_test.cpp — Phase 11 Wave 11D (ADR-0026).

#include <doctest/doctest.h>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/standard_cvars.hpp"
#include "engine/core/events/event_bus.hpp"
#include "engine/ui/font_library.hpp"
#include "engine/ui/glyph_atlas.hpp"
#include "engine/ui/locale_bridge.hpp"
#include "engine/ui/text_shaper.hpp"
#include "engine/ui/ui_service.hpp"

using namespace gw::ui;

namespace {
struct Fixture {
    gw::config::CVarRegistry   cvars{};
    gw::config::StandardCVars  std_cvars{};
    FontLibrary  fonts{};
    GlyphAtlas   atlas{512};
    TextShaper   shaper{};
    LocaleBridge locale{};
    gw::events::EventBus<gw::events::UILayoutComplete> layout_bus{};
    gw::events::EventBus<gw::events::WindowResized>    resize_bus{};

    Fixture() { std_cvars = gw::config::register_standard_cvars(cvars); }
};
} // namespace

TEST_CASE("UIService: constructs and ticks without RmlUi") {
    Fixture fx;
    UIService svc({1280, 720, 1.0f, "ui.default", false},
                  fx.cvars, fx.fonts, fx.atlas, fx.shaper, fx.locale,
                  &fx.layout_bus, &fx.resize_bus);
    CHECK(svc.document_count() == 0);
    svc.update(1.0 / 60.0);
    svc.render();
    CHECK(svc.stats().frame_index == 1);
}

TEST_CASE("UIService: load_document assigns fresh handles") {
    Fixture fx;
    UIService svc({1280, 720, 1.0f, "ui.default", false},
                  fx.cvars, fx.fonts, fx.atlas, fx.shaper, fx.locale);
    DocumentHandle a{}, b{};
    CHECK(svc.load_document("assets/ui/hud.rml", a));
    CHECK(svc.load_document("assets/ui/menu.rml", b));
    CHECK(a.valid());
    CHECK(b.valid());
    CHECK(a != b);
    CHECK(svc.document_count() == 2);
    CHECK(svc.unload_document(a));
    CHECK(svc.document_count() == 1);
}

TEST_CASE("UIService: set_viewport publishes WindowResized") {
    Fixture fx;
    UIService svc({1280, 720, 1.0f, "ui.default", false},
                  fx.cvars, fx.fonts, fx.atlas, fx.shaper, fx.locale,
                  &fx.layout_bus, &fx.resize_bus);
    int hits = 0;
    (void)fx.resize_bus.subscribe([&](const gw::events::WindowResized& e) {
        CHECK(e.width == 3840);
        CHECK(e.height == 2160);
        ++hits;
    });
    svc.set_viewport(3840, 2160, 2.0f);
    CHECK(hits == 1);
}

TEST_CASE("UIService: render publishes UILayoutComplete once per frame") {
    Fixture fx;
    UIService svc({1280, 720, 1.0f, "ui.default", false},
                  fx.cvars, fx.fonts, fx.atlas, fx.shaper, fx.locale,
                  &fx.layout_bus, &fx.resize_bus);
    int hits = 0;
    (void)fx.layout_bus.subscribe([&](const gw::events::UILayoutComplete&) { ++hits; });
    svc.update(1.0 / 60.0);
    svc.render();
    svc.update(1.0 / 60.0);
    svc.render();
    CHECK(hits == 2);
}
