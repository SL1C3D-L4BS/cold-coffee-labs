#pragma once

#include "engine/services/gore/schema/gore.hpp"

namespace gw::services::gore {

[[nodiscard]] GoreHitResult apply_hit(const GoreHitRequest& req) noexcept;

} // namespace gw::services::gore
