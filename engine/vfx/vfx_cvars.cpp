// engine/vfx/vfx_cvars.cpp — ADR-0077/0078.

#include "engine/vfx/vfx_cvars.hpp"

#include "engine/core/config/cvar_registry.hpp"

namespace gw::vfx {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kU = config::kCVarUserFacing;
} // namespace

VfxCVars register_vfx_cvars(config::CVarRegistry& r) {
    VfxCVars c{};
    c.particles_budget = r.register_i32({
        "vfx.particles.budget", 1 << 20, kP, 1024, 1 << 22, true,
        "GPU particle budget (default 1M, ADR-0077).",
    });
    c.particles_sort_mode = r.register_string({
        "vfx.particles.sort_mode", std::string{"none"}, kP | kU, {}, {}, false,
        "Particle alpha sort mode: none | bitonic (ADR-0077 §7).",
    });
    c.particles_curl_noise_octaves = r.register_i32({
        "vfx.particles.curl_noise_octaves", 3, kP, 1, 8, true,
        "Curl-noise octaves for particle turbulence.",
    });
    c.ribbons_budget = r.register_i32({
        "vfx.ribbons.budget", 128, kP, 1, 1024, true,
        "Maximum concurrent ribbon states.",
    });
    c.ribbons_max_length = r.register_i32({
        "vfx.ribbons.max_length", 256, kP, 2, 4096, true,
        "Max vertex-pair count per ribbon state.",
    });
    c.decals_budget = r.register_i32({
        "vfx.decals.budget", 512, kP, 1, 8192, true,
        "Concurrent screen-space decals (ADR-0078).",
    });
    c.decals_cluster_bin = r.register_i32({
        "vfx.decals.cluster_bin", 16, kP, 1, 128, true,
        "Tile size (px) for decal cluster binning.",
    });
    c.gpu_async = r.register_bool({
        "vfx.gpu_async", true, kP, {}, {}, false,
        "Run particle/ribbon/decal dispatches on vfx_gpu job lane.",
    });
    return c;
}

} // namespace gw::vfx
