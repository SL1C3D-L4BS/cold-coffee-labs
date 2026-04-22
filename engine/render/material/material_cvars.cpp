// engine/render/material/material_cvars.cpp — ADR-0075 §5, ADR-0074 §5.

#include "engine/render/material/material_cvars.hpp"

namespace gw::render::material {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kU = config::kCVarUserFacing;
constexpr std::uint32_t kD = config::kCVarDevOnly;
} // namespace

MaterialCVars register_material_cvars(config::CVarRegistry& r) {
    MaterialCVars c{};
    c.default_template = r.register_string({
        "mat.default_template", std::string{"pbr_opaque/metal_rough"}, kP | kU, {}, {}, false,
        "Default material template for newly-created instances.",
    });
    c.instance_budget = r.register_i32({
        "mat.instance_budget", 4096, kP, 64, 65536, true,
        "Maximum concurrent material instances.",
    });
    c.texture_cache_mb = r.register_i32({
        "mat.texture_cache_mb", 512, kP, 64, 4096, true,
        "Texture-slot cache budget (MB).",
    });
    c.auto_reload = r.register_bool({
        "mat.auto_reload", true, kP | kU, {}, {}, false,
        "Auto-reload queued instance changes during step().",
    });
    c.preview_sphere = r.register_bool({
        "mat.preview_sphere", true, kP | kU, {}, {}, false,
        "Editor preview sphere for material inspection.",
    });
    c.quality_tier = r.register_string({
        "mat.quality_tier", std::string{"high"}, kP | kU, {}, {}, false,
        "Material quality tier: low | med | high.",
    });
    c.sheen_enabled = r.register_bool({
        "mat.sheen_enabled", true, kP | kU, {}, {}, false,
        "Gate KHR_materials_sheen permutation axis.",
    });
    c.transmission_enabled = r.register_bool({
        "mat.transmission_enabled", true, kP | kU, {}, {}, false,
        "Gate KHR_materials_transmission permutation axis.",
    });
    return c;
}

ShaderCVars register_shader_cvars(config::CVarRegistry& r) {
    ShaderCVars c{};
    c.slang_enabled = r.register_bool({
        "r.shader.slang_enabled", false, kP | kU, {}, {}, false,
        "Runtime probe of GW_ENABLE_SLANG build gate (ADR-0074).",
    });
    c.permutation_budget = r.register_i32({
        "r.shader.permutation_budget", 64, kP, 1, 4096, true,
        "Hard ceiling per template (ADR-0074 §2).",
    });
    c.cache_max_mb = r.register_i32({
        "r.shader.cache_max_mb", 256, kP, 16, 4096, true,
        "Permutation cache LRU cap (MB).",
    });
    c.hot_reload = r.register_bool({
        "r.shader.hot_reload", true, kP | kU, {}, {}, false,
        "File-watch driven shader hot-reload.",
    });
    c.sm69_emulate = r.register_bool({
        "r.shader.sm69_emulate", false, kD, {}, {}, false,
        "Force the SM 6.6 fallback path for CI coverage (dev-only).",
    });
    return c;
}

} // namespace gw::render::material
