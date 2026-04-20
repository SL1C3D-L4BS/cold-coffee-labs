#include "engine/core/version.hpp"

namespace gw::core {

const char* version_string() noexcept {
    // Stored in .rodata; 'static lifetime. Safe as a C string.
    static constexpr const char kVersionString[] =
        "Greywater_Engine " GW_VERSION_STRING " (build=" GW_BUILD_TYPE ")";
    return kVersionString;
}

}  // namespace gw::core
