// tests/unit/ui/font_library_test.cpp — Phase 11 Wave 11D (ADR-0028).

#include <doctest/doctest.h>

#include "engine/ui/font_library.hpp"

using namespace gw::ui;

TEST_CASE("FontLibrary: register + face lookup") {
    FontLibrary lib;
    FontFace f;
    f.family = "Inter";
    f.style  = "Regular";
    f.path   = "assets/fonts/Inter-Regular.ttf";
    f.weight = 400;
    auto id = lib.register_face(f);
    CHECK(id.valid());
    CHECK(lib.face_count() == 1);
    auto* got = lib.face(id);
    REQUIRE(got != nullptr);
    CHECK(got->family == "Inter");
}

TEST_CASE("FontLibrary: load_face accumulates bytes_loaded") {
    FontLibrary lib;
    auto id = lib.register_face({"Noto", "Regular", "", 400, false, 1.2f});
    std::vector<std::byte> blob(1024, std::byte{0x42});
    CHECK(lib.load_face(id, std::span<const std::byte>{blob.data(), blob.size()}));
    CHECK(lib.bytes_loaded() == 1024);
}

TEST_CASE("FontLibrary: fallback chain round-trip") {
    FontLibrary lib;
    auto a = lib.register_face({"A", "R", "", 400, false, 1.2f});
    auto b = lib.register_face({"B", "R", "", 400, false, 1.2f});
    FontFaceId order[] = {a, b};
    lib.set_fallback_chain("ui.default", std::span<const FontFaceId>{order, 2});
    auto* chain = lib.chain("ui.default");
    REQUIRE(chain != nullptr);
    CHECK(chain->order.size() == 2);
    CHECK(chain->order[0] == a);
    CHECK(chain->order[1] == b);
}
