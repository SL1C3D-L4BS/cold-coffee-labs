#pragma once

#include <filesystem>
#include <string_view>

namespace gw::persist {
class ILocalStore;
}

namespace gw::telemetry {

[[nodiscard]] bool dsar_export_to_dir(const std::filesystem::path& dir,
                                      gw::persist::ILocalStore&   store,
                                      std::string_view            user_id_hash_hex);

} // namespace gw::telemetry
