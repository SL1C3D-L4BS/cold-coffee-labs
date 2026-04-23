#pragma once

#include "engine/services/level_architect/schema/layout.hpp"

namespace gw::services::level_architect {

/// Generate a layout deterministically. Shim wraps `engine/world/` + WFC.
[[nodiscard]] LayoutResult generate(const LayoutRequest& req) noexcept;

} // namespace gw::services::level_architect
