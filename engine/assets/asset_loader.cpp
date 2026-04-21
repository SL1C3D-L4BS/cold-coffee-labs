#include "asset_loader.hpp"

namespace gw::assets {

const AssetLoaderEntry* AssetLoaderRegistry::find(AssetType type) const noexcept {
    auto it = map_.find(static_cast<uint16_t>(type));
    return (it != map_.end()) ? &it->second : nullptr;
}

} // namespace gw::assets
