#pragma once
// engine/vfx/vfx_cvars.hpp — ADR-0077/0078.

#include "engine/core/config/cvar_registry.hpp"

namespace gw::vfx {

struct VfxCVars {
    config::CVarRef<std::int32_t> particles_budget{};
    config::CVarRef<std::string>  particles_sort_mode{};
    config::CVarRef<std::int32_t> particles_curl_noise_octaves{};
    config::CVarRef<std::int32_t> ribbons_budget{};
    config::CVarRef<std::int32_t> ribbons_max_length{};
    config::CVarRef<std::int32_t> decals_budget{};
    config::CVarRef<std::int32_t> decals_cluster_bin{};
    config::CVarRef<bool>         gpu_async{};
};

[[nodiscard]] VfxCVars register_vfx_cvars(config::CVarRegistry& r);

} // namespace gw::vfx
