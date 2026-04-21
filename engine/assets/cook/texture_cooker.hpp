#pragma once
// engine/assets/cook/texture_cooker.hpp
// TextureCooker: PNG/EXR/TGA → KTX2 (UASTC) → .gwtex
// Phase 6 spec §7 (Week 035).

#include "cook_registry.hpp"

namespace gw::assets::cook {

class TextureCooker final : public IAssetCooker {
public:
    [[nodiscard]] AssetResult<CookResult> cook(const CookContext& ctx) const override;
    [[nodiscard]] std::vector<std::string> extensions() const override {
        return {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr", ".exr"};
    }
    [[nodiscard]] std::string name() const override { return "TextureCooker"; }
    [[nodiscard]] uint32_t    rule_version() const override;
};

} // namespace gw::assets::cook
