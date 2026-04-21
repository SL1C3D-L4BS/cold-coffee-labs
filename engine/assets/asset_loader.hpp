#pragma once
// engine/assets/asset_loader.hpp
// IAssetLoader interface + typed loader registry.

#include "asset_handle.hpp"
#include "asset_error.hpp"
#include <functional>
#include <memory>
#include <span>
#include <unordered_map>

namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

// Called once the raw cooked bytes have been read from disk.
// The loader is responsible for parsing and uploading to GPU.
// Returns an owning void* that the AssetDatabase stores in the slot.
using AssetLoadFn = std::function<AssetResult<void*>(
    render::hal::VulkanDevice& device,
    std::span<const uint8_t> raw)>;

using AssetDestroyFn = std::function<void(void* data)>;

struct AssetLoaderEntry {
    AssetLoadFn    load;
    AssetDestroyFn destroy;
};

// Registry that maps AssetType → loader/destroy pairs.
class AssetLoaderRegistry {
public:
    template<typename T>
    void register_loader();

    [[nodiscard]] const AssetLoaderEntry* find(AssetType type) const noexcept;

private:
    std::unordered_map<uint16_t, AssetLoaderEntry> map_;
};

// ---------------------------------------------------------------------------
// Template implementation (header-only — instantiated per asset type)
// ---------------------------------------------------------------------------
template<typename T>
void AssetLoaderRegistry::register_loader() {
    AssetLoaderEntry entry;

    entry.load = [](render::hal::VulkanDevice& device,
                    std::span<const uint8_t> raw) -> AssetResult<void*> {
        auto* asset = new T(); // deleted by entry.destroy
        auto result = asset->upload(device, raw);
        if (!result) {
            delete asset;
            return std::unexpected(result.error());
        }
        return static_cast<void*>(asset);
    };

    entry.destroy = [](void* data) {
        delete static_cast<T*>(data);
    };

    map_[T::kAssetTypeTag] = std::move(entry);
}

} // namespace gw::assets
