#pragma once

#include "engine/services/director/schema/director.hpp"

namespace gw::services::director {

[[nodiscard]] DirectorResult evaluate(const DirectorRequest& req) noexcept;

} // namespace gw::services::director
