#pragma once

#include <array>
#include <cstdint>

namespace gw::services::material_forge {

/// POD schema, IP-agnostic. No Sacrilege-specific enums / strings.
struct MaterialId { std::uint32_t value = 0; };

struct MaterialParamsPOD {
    std::array<float, 3> base_color{1.f, 1.f, 1.f};
    float roughness  = 0.5f;
    float metallic   = 0.f;
    float wear       = 0.f;
    float wetness    = 0.f;
    float blood      = 0.f;
    float emissive   = 0.f;
    std::uint32_t flags = 0;   // bitfield defined per consumer
};

struct MaterialEvalRequest {
    MaterialId        material{};
    MaterialParamsPOD params{};
    std::uint64_t     seed = 0;
};

struct MaterialEvalResult {
    std::array<float, 3> albedo{1.f, 1.f, 1.f};
    std::array<float, 3> normal{0.f, 0.f, 1.f};
    float                roughness = 0.5f;
    float                metallic  = 0.f;
    float                ao        = 1.f;
};

} // namespace gw::services::material_forge
