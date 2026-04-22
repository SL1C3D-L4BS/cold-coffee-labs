#pragma once
// engine/render/material/material_config.hpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_types.hpp"

#include <cstdint>
#include <string>

namespace gw::render::material {

struct MaterialConfig {
    std::string         default_template{"pbr_opaque/metal_rough"};
    std::uint32_t       instance_budget{4096};
    std::uint32_t       texture_cache_mb{512};
    bool                auto_reload{true};
    bool                preview_sphere{true};
    MaterialQualityTier quality{MaterialQualityTier::High};
    bool                sheen_enabled{true};
    bool                transmission_enabled{true};
};

} // namespace gw::render::material
