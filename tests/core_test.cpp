#include "engine/core/error.hpp"
#include "engine/core/result.hpp"

#include <doctest/doctest.h>
#include <ostream>

TEST_CASE("core/error exposes stable error code names") {
    CHECK(gw::core::error_code_name(gw::core::ErrorCode::InvalidArgument) == "InvalidArgument");
}

TEST_CASE("core/result transports values and errors") {
    gw::core::Result<int, gw::core::Error> ok{42};
    REQUIRE(ok.has_value());
    CHECK(ok.value() == 42);

    gw::core::Result<int, gw::core::Error> err{
        gw::core::Error{gw::core::ErrorCode::NotFound, "missing"}};
    REQUIRE_FALSE(err.has_value());
    CHECK(err.error().code == gw::core::ErrorCode::NotFound);
}
