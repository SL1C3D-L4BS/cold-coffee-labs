#pragma once

#include <string>

namespace gw::telemetry {

[[nodiscard]] std::string scrub_pii(std::string_view in, bool scrub_paths);

} // namespace gw::telemetry
