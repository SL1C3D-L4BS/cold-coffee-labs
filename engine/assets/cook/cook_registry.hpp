#pragma once
// engine/assets/cook/cook_registry.hpp
// IAssetCooker interface + CookRegistry that maps file extensions to cookers.

#include "../asset_types.hpp"
#include "../asset_error.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gw::assets::cook {

// Context passed into every cooker invocation.
struct CookContext {
    std::filesystem::path source_path;
    std::filesystem::path output_path;
    std::filesystem::path cache_dir;
    CookPlatform          platform = CookPlatform::Host;
    CookConfig            config   = CookConfig::Release;
    bool                  verbose  = false;
};

// Output of a single cook run.
struct CookResult {
    CookKey  cook_key;
    uint32_t cook_time_ms  = 0;
    bool     cache_hit     = false;
    std::vector<AssetPath> deps;
};

// Interface every asset type cooker implements.
class IAssetCooker {
public:
    virtual ~IAssetCooker() = default;

    // Cook the asset described by ctx.
    // Returns the result on success or an AssetError on failure.
    [[nodiscard]] virtual AssetResult<CookResult>
    cook(const CookContext& ctx) const = 0;

    // File extensions this cooker handles (e.g. {".glb", ".gltf"}).
    [[nodiscard]] virtual std::vector<std::string> extensions() const = 0;

    // Human-readable name, used in manifest and logging.
    [[nodiscard]] virtual std::string name() const = 0;

    // Version bumped when the cook transform logic changes.
    [[nodiscard]] virtual uint32_t rule_version() const = 0;
};

// Registry that maps file extensions → cooker instances.
class CookRegistry {
public:
    void register_cooker(std::unique_ptr<IAssetCooker> cooker);

    // Find a cooker for the given extension (lowercase, with leading '.').
    // Returns nullptr if no cooker is registered.
    [[nodiscard]] const IAssetCooker* find(const std::string& ext) const noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<IAssetCooker>>& all() const noexcept {
        return cookers_;
    }

private:
    std::vector<std::unique_ptr<IAssetCooker>> cookers_;
    std::unordered_map<std::string, const IAssetCooker*> ext_map_;
};

// Build the default registry with all built-in cookers registered.
[[nodiscard]] CookRegistry make_default_registry();

} // namespace gw::assets::cook
