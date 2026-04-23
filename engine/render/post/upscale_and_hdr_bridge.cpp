// engine/render/post/upscale_and_hdr_bridge.cpp — pre-eng-fsr2-hal-frame-graph.

#include "engine/render/post/upscale_and_hdr_bridge.hpp"

namespace gw::render::post {

UpscaleHdrResult apply_upscale_and_hdr(const UpscaleHdrConfig& cfg) noexcept {
    UpscaleHdrResult r{};
    if (cfg.fsr2_enabled) {
        fsr2::Fsr2Params p{};
        p.mode = cfg.fsr2_mode;
        p.hdr  = cfg.hdr_enabled;
        r.fsr2_out = fsr2::dispatch(p);
    }
    if (cfg.hdr_enabled && gw::render::hal::hdr10_supported()) {
        r.hdr_applied = gw::render::hal::apply_hdr10_metadata(cfg.hdr);
    }
    return r;
}

} // namespace gw::render::post
