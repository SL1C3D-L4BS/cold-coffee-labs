// tests/smoke_test.cpp — Phase 1 smoke coverage on the engine-core surface.
//
// These tests prove:
//   - greywater_core compiles and links under Clang on Windows + Linux,
//   - the version surface is reachable from an external TU,
//   - doctest CMake integration auto-registers cases with CTest.

#include "engine/core/version.hpp"

#include <doctest/doctest.h>

#include <cstring>
#include <string_view>

TEST_CASE("engine/core/version: exposes non-null, non-empty version string") {
    const char* v = gw::core::version_string();
    REQUIRE(v != nullptr);
    CHECK(std::strlen(v) > 0);
    CHECK(std::string_view(v).find("Greywater_Engine") != std::string_view::npos);
}

TEST_CASE("engine/core/version: compile-time version matches project() at configure") {
    constexpr auto v = gw::core::version();
    static_assert(v.major == GW_VERSION_MAJOR);
    static_assert(v.minor == GW_VERSION_MINOR);
    static_assert(v.patch == GW_VERSION_PATCH);
    CHECK(v.major == GW_VERSION_MAJOR);
    CHECK(v.minor == GW_VERSION_MINOR);
    CHECK(v.patch == GW_VERSION_PATCH);
}
