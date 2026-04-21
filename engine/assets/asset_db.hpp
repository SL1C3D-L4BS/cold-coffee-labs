#pragma once
// engine/assets/asset_db.hpp
// AssetDatabase — typed generational handle database with async load and
// hot-reload support.  Phase 6 spec §8.2.

#include "asset_handle.hpp"
#include "asset_error.hpp"
#include "asset_loader.hpp"
#include "asset_types.hpp"
#include "mesh_asset.hpp"
#include "texture_asset.hpp"
#include "vfs/virtual_fs.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

enum class AssetState : uint8_t {
    Unloaded,
    Pending,     // IO + decode in flight
    Uploading,   // GPU transfer in flight
    Ready,       // fully GPU-resident and safe to use
    Evicted,     // was resident; VMA memory returned
    Error,
};

struct AssetMetadata {
    AssetPath   source_path;
    AssetPath   cooked_path;
    CookKey     cook_key;
    uint64_t    size_bytes  = 0;
    AssetState  state       = AssetState::Unloaded;
    AssetType   type        = AssetType::Unknown;
    uint16_t    generation  = 0;
    uint32_t    ref_count   = 0;
};

class AssetDatabase {
public:
    explicit AssetDatabase(render::hal::VulkanDevice& device,
                            vfs::VirtualFilesystem&    vfs);
    ~AssetDatabase();

    AssetDatabase(const AssetDatabase&)            = delete;
    AssetDatabase& operator=(const AssetDatabase&) = delete;

    // -----------------------------------------------------------------------
    // Synchronous load — blocks the calling thread.  Editor / tool use only.
    // -----------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] AssetResult<TypedHandle<T>>
    load_sync(const AssetPath& cooked_path);

    // -----------------------------------------------------------------------
    // Async load — returns immediately.  The handle is valid but state is
    // Pending until tick() drives it to Ready.
    // -----------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] TypedHandle<T> load_async(const AssetPath& cooked_path);

    // -----------------------------------------------------------------------
    // Access resident data — returns nullptr if not Ready.
    // -----------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] const T* get(TypedHandle<T> handle) const noexcept;

    // Ref-counting (optional; use for streaming/eviction in Phases 19+).
    void retain(AssetHandle h);
    void release(AssetHandle h);

    // Invalidate all handles to a cooked path and re-queue a load.
    // Called by FSWatcher on hot-reload events.
    void invalidate(const AssetPath& cooked_path);

    // Register an event callback fired when an asset transitions to Ready.
    using ReadyCallback = std::function<void(AssetHandle)>;
    void on_ready(ReadyCallback cb) { ready_cb_ = std::move(cb); }

    // Process completed async IO; fire ReadyCallbacks.
    // Call once per frame from the main thread.
    void tick();

    // Build the default loader registry.
    static AssetLoaderRegistry make_default_registry();

private:
    struct Slot {
        AssetMetadata meta;
        void*         data_ptr = nullptr;
    };

    uint32_t alloc_slot();
    void     free_slot(uint32_t idx);

    AssetHandle register_or_find(const AssetPath& cooked_path, AssetType type);
    void        enqueue_load(AssetHandle h);
    void        loader_thread_fn();

    render::hal::VulkanDevice&         device_;
    vfs::VirtualFilesystem&            vfs_;
    AssetLoaderRegistry                loaders_;

    mutable std::mutex                 mutex_;
    std::vector<Slot>                  slots_;
    std::vector<uint32_t>              free_list_;
    std::unordered_map<AssetPath, uint32_t> path_to_index_;

    // Async loader thread state.
    struct LoadRequest {
        AssetHandle handle;
        AssetPath   cooked_path;
    };
    std::vector<LoadRequest>    pending_queue_;
    std::vector<LoadRequest>    completed_queue_;
    mutable std::mutex          queue_mutex_;

    std::thread                 loader_thread_;
    std::atomic<bool>           stop_loader_{false};

    ReadyCallback               ready_cb_;
};

// ---------------------------------------------------------------------------
// Template implementations
// ---------------------------------------------------------------------------

template<typename T>
AssetResult<TypedHandle<T>> AssetDatabase::load_sync(const AssetPath& cooked_path) {
    AssetHandle h = register_or_find(cooked_path, static_cast<AssetType>(T::kAssetTypeTag));

    std::vector<uint8_t> raw;
    auto read_res = vfs_.read(cooked_path, raw);
    if (!read_res) {
        std::lock_guard lock{mutex_};
        slots_[h.index()].meta.state = AssetState::Error;
        return std::unexpected(read_res.error());
    }

    const auto* loader = loaders_.find(static_cast<AssetType>(T::kAssetTypeTag));
    if (!loader) {
        return std::unexpected(AssetError{AssetErrorCode::InvalidArgument,
                                          "no loader for asset type"});
    }

    auto result = loader->load(device_, raw);
    if (!result) {
        std::lock_guard lock{mutex_};
        slots_[h.index()].meta.state = AssetState::Error;
        return std::unexpected(result.error());
    }

    {
        std::lock_guard lock{mutex_};
        auto& slot     = slots_[h.index()];
        slot.data_ptr  = *result;
        slot.meta.state= AssetState::Ready;
    }

    if (ready_cb_) ready_cb_(h);
    return TypedHandle<T>{h};
}

template<typename T>
TypedHandle<T> AssetDatabase::load_async(const AssetPath& cooked_path) {
    AssetHandle h = register_or_find(cooked_path, static_cast<AssetType>(T::kAssetTypeTag));
    enqueue_load(h);
    return TypedHandle<T>{h};
}

template<typename T>
const T* AssetDatabase::get(TypedHandle<T> handle) const noexcept {
    if (!handle.valid()) return nullptr;
    std::lock_guard lock{mutex_};
    if (handle.index() >= slots_.size()) return nullptr;
    const auto& slot = slots_[handle.index()];
    if (slot.meta.generation != handle.generation()) return nullptr;
    if (slot.meta.state != AssetState::Ready) return nullptr;
    return static_cast<const T*>(slot.data_ptr);
}

} // namespace gw::assets
