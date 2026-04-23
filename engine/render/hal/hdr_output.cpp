// engine/render/hal/hdr_output.cpp — Part C §21 scaffold.

#include "engine/render/hal/hdr_output.hpp"

namespace gw::render::hal {

bool hdr10_supported() noexcept {
    // Phase 22: probe VK_EXT_hdr_metadata + surface format list. Scaffold
    // returns false so CI behaves as SDR until the real probe lands.
    return false;
}

bool apply_hdr10_metadata(const Hdr10Metadata& /*md*/) noexcept {
    return false;
}

} // namespace gw::render::hal
