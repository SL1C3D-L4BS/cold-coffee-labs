#include "cook_registry.hpp"
#include "mesh_cooker.hpp"
#include "texture_cooker.hpp"
#include <algorithm>
#include <stdexcept>

namespace gw::assets::cook {

void CookRegistry::register_cooker(std::unique_ptr<IAssetCooker> cooker) {
    const auto* raw = cooker.get();
    for (const auto& ext : cooker->extensions()) {
        std::string lower = ext;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        ext_map_[lower] = raw;
    }
    cookers_.push_back(std::move(cooker));
}

const IAssetCooker* CookRegistry::find(const std::string& ext) const noexcept {
    auto it = ext_map_.find(ext);
    return (it != ext_map_.end()) ? it->second : nullptr;
}

CookRegistry make_default_registry() {
    CookRegistry reg;
    reg.register_cooker(std::make_unique<MeshCooker>());
    reg.register_cooker(std::make_unique<TextureCooker>());
    return reg;
}

} // namespace gw::assets::cook
