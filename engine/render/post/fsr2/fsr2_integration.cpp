// engine/render/post/fsr2/fsr2_integration.cpp — Part C §21 scaffold.

#include "engine/render/post/fsr2/fsr2_integration.hpp"

namespace gw::render::post::fsr2 {

Fsr2Dispatch dispatch(const Fsr2Params& params) noexcept {
    Fsr2Dispatch out{};
    out.ok = params.mode != UpscaleMode::Off;
    return out;
}

} // namespace gw::render::post::fsr2
