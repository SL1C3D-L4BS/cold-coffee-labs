#pragma once
// engine/assets/cook/mesh_cooker.hpp
// MeshCooker: glTF 2.0 → .gwmesh via fastgltf + MikkTSpace + meshoptimizer.
// Phase 6 spec §6 (Week 034).

#include "cook_registry.hpp"

namespace gw::assets::cook {

class MeshCooker final : public IAssetCooker {
public:
    [[nodiscard]] AssetResult<CookResult> cook(const CookContext& ctx) const override;
    [[nodiscard]] std::vector<std::string> extensions() const override {
        return {".glb", ".gltf"};
    }
    [[nodiscard]] std::string name() const override { return "MeshCooker"; }
    [[nodiscard]] uint32_t    rule_version() const override;
};

} // namespace gw::assets::cook
