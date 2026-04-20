#include "engine/core/string.hpp"

#include <doctest/doctest.h>
#include <ostream>

TEST_CASE("core/string uses sso for small strings") {
    gw::core::Utf8String str{"hello"};
    CHECK(str.uses_sso());
    CHECK(str.view() == "hello");
}

TEST_CASE("core/string uses heap for long strings") {
    gw::core::Utf8String str{"this is definitely longer than the sso capacity"};
    CHECK_FALSE(str.uses_sso());
}

TEST_CASE("core/string validates utf8 input") {
    gw::core::Utf8String str{};
    CHECK(str.assign("valid utf8 text"));
    const std::string invalid = std::string("\xC3\x28", 2);
    CHECK_FALSE(str.assign(invalid));
}
