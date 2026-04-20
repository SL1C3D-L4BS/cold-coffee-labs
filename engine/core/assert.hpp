#pragma once

#include <string_view>

namespace gw::core {

[[noreturn]] void assert_fail(const char* expr, const char* file, int line, std::string_view message);

}  // namespace gw::core

#define GW_ASSERT(expr, msg)                                    \
    do {                                                        \
        if (!(expr)) {                                          \
            ::gw::core::assert_fail(#expr, __FILE__, __LINE__, (msg)); \
        }                                                       \
    } while (false)
