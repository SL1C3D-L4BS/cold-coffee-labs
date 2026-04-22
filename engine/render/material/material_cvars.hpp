#pragma once
// engine/render/material/material_cvars.hpp — ADR-0075 §5.

#include "engine/core/config/cvar_registry.hpp"

namespace gw::render::material {

struct MaterialCVars {
    config::CVarRef<std::string>  default_template{};
    config::CVarRef<std::int32_t> instance_budget{};
    config::CVarRef<std::int32_t> texture_cache_mb{};
    config::CVarRef<bool>         auto_reload{};
    config::CVarRef<bool>         preview_sphere{};
    config::CVarRef<std::string>  quality_tier{};
    config::CVarRef<bool>         sheen_enabled{};
    config::CVarRef<bool>         transmission_enabled{};
};

[[nodiscard]] MaterialCVars register_material_cvars(config::CVarRegistry& r);

// Wave 17A shader CVars live alongside material (same library, same header-
// quarantine story) so runtime can register both in one call.
struct ShaderCVars {
    config::CVarRef<bool>         slang_enabled{};
    config::CVarRef<std::int32_t> permutation_budget{};
    config::CVarRef<std::int32_t> cache_max_mb{};
    config::CVarRef<bool>         hot_reload{};
    config::CVarRef<bool>         sm69_emulate{};
};

[[nodiscard]] ShaderCVars register_shader_cvars(config::CVarRegistry& r);

} // namespace gw::render::material
