// tests/unit/ui/locale_bridge_test.cpp — Phase 11 Wave 11D (ADR-0028).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/ui/locale_bridge.hpp"

using namespace gw::ui;

TEST_CASE("LocaleBridge: register + resolve hits the matching locale") {
    LocaleBridge lb;
    lb.register_string("en-US", "menu.quit", "Quit");
    auto id = LocaleBridge::make_id("menu.quit");
    CHECK(lb.resolve(id) == "Quit");
}

TEST_CASE("LocaleBridge: falls back to en-US when key missing in current locale") {
    LocaleBridge lb;
    lb.register_string("en-US", "hud.health", "Health");
    lb.register_string("fr-FR", "menu.quit",  "Quitter");
    CHECK(lb.set_locale("fr-FR"));
    auto id = LocaleBridge::make_id("hud.health");
    CHECK(lb.resolve(id) == "Health");
}

TEST_CASE("LocaleBridge: missing key returns fallback literal") {
    LocaleBridge lb;
    auto id = LocaleBridge::make_id("unknown.key");
    CHECK(lb.resolve(id, "unknown.key") == "unknown.key");
}

TEST_CASE("LocaleBridge: re-registration updates the value") {
    LocaleBridge lb;
    lb.register_string("en-US", "menu.quit", "Quit");
    lb.register_string("en-US", "menu.quit", "Exit");
    CHECK(lb.resolve(LocaleBridge::make_id("menu.quit")) == "Exit");
}

TEST_CASE("LocaleBridge: set_locale to unknown tag falls back to en-US") {
    LocaleBridge lb;
    lb.register_string("en-US", "menu.quit", "Quit");
    CHECK_FALSE(lb.set_locale("xx-XX"));
    CHECK(lb.locale() == "en-US");
}
