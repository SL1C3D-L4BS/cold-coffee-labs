#pragma once
// engine/render/material/events_material.hpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_types.hpp"

#include <cstdint>
#include <string>

namespace gw::render::material {

struct MaterialInstanceCreated {
    MaterialTemplateId parent{};
    MaterialInstanceId id{};
    std::uint64_t      timestamp_ns{0};
};

struct MaterialInstanceDestroyed {
    MaterialInstanceId id{};
    std::uint64_t      timestamp_ns{0};
};

struct MaterialReloaded {
    MaterialTemplateId parent{};
    MaterialInstanceId id{};        // {0} if template-wide reload
    std::uint64_t      timestamp_ns{0};
    std::string        reason;       // "hlsl" | "gwmat" | "manual"
};

struct MaterialParamChanged {
    MaterialInstanceId id{};
    std::string        key;
    std::uint32_t      revision{0};
};

} // namespace gw::render::material
