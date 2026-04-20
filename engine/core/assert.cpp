#include "engine/core/assert.hpp"

#include <cstdlib>
#include <iostream>

namespace gw {
namespace core {

[[noreturn]] void assert_fail(const char* expr, const char* file, int line, std::string_view message) {
    std::cerr << "GW_ASSERT failed: " << expr << " at " << file << ":" << line
              << " message=" << message << "\n";
    std::abort();
}

}  // namespace core
}  // namespace gw
