#pragma once
// engine/assets/asset_loader.hpp
// IAssetLoader interface + typed loader registry.
// No raw new/delete — assets are owned by unique_ptr; ownership is transferred
// to the AssetDatabase slot via a type-erased deleter stored alongside data_ptr.

#include "asset_handle.hpp"
#include "asset_error.hpp"
#include <functional>
#include <memory>
#include <span>
#include <unordered_map>

namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

// LoadResult carries an owning void* together with a matching destroy callback.
struct LoadedAsset {
    void*                     data    = nullptr;
    std::function<void(void*)> destroy;

    LoadedAsset() = default;
    LoadedAsset(void* d, std::function<void(void*)> fn) noexcept
        : data(d), destroy(std::move(fn)) {}

    LoadedAsset(const LoadedAsset&)            = delete;
    LoadedAsset& operator=(const LoadedAsset&) = delete;

    LoadedAsset(LoadedAsset&& o) noexcept
        : data(o.data), destroy(std::move(o.destroy)) { o.data = nullptr; }
    LoadedAsset& operator=(LoadedAsset&& o) noexcept {
        if (this != &o) {
            release();
            data    = o.data;
            destroy = std::move(o.destroy);
            o.data  = nullptr;
        }
        return *this;
    }

    ~LoadedAsset() { release(); }

    void release() noexcept {
        if (data && destroy) {
            destroy(data);
            data = nullptr;
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept { return data != nullptr; }
};

using AssetLoadFn = std::function<AssetResult<LoadedAsset>(
    render::hal::VulkanDevice& device,
    std::span<const uint8_t>   raw)>;

struct AssetLoaderEntry {
    AssetLoadFn load;
};

// Registry that maps AssetType → loader.
class AssetLoaderRegistry {
public:
    template<typename T>
    void register_loader();

    [[nodiscard]] const AssetLoaderEntry* find(AssetType type) const noexcept;

private:
    std::unordered_map<uint16_t, AssetLoaderEntry> map_;
};

// ---------------------------------------------------------------------------
// Template implementation — instantiated per asset type T.
// T must provide:
//   static constexpr uint16_t kAssetTypeTag;
//   AssetResult<void> upload(VulkanDevice&, std::span<const uint8_t>);
// ---------------------------------------------------------------------------
template<typename T>
void AssetLoaderRegistry::register_loader() {
    AssetLoaderEntry entry;

    entry.load = [](render::hal::VulkanDevice& device,
                    std::span<const uint8_t>   raw) -> AssetResult<LoadedAsset> {
        auto asset = std::make_unique<T>();
        auto result = asset->upload(device, raw);
        if (!result) return std::unexpected(result.error());

        // Transfer ownership via a typed deleter — no raw delete.
        T* raw_ptr = asset.release();
        return LoadedAsset{
            static_cast<void*>(raw_ptr),
            [](void* p) noexcept { delete static_cast<T*>(p); }
        };
    };

    map_[T::kAssetTypeTag] = std::move(entry);
}

}  // namespace gw::assets
