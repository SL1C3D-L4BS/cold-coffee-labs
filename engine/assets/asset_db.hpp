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
#include "engine/jobs/reserved_worker.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace gw::render::hal { class VulkanDevice; }

namespace gw::assets {

/// Tag: mod/studio sandboxes with no GPU device. No background loader; `load_sync` returns an error
/// for GPU-backed types until a full `AssetDatabase(VulkanDevice&, …)` is used.
struct AssetDatabaseModHarnessTag {
    explicit AssetDatabaseModHarnessTag() = default;
};

enum class AssetState : uint8_t {
    Unloaded,
    Pending,
    Uploading,
    Ready,
    Evicted,
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
    explicit AssetDatabase(AssetDatabaseModHarnessTag, vfs::VirtualFilesystem& vfs) noexcept;
    ~AssetDatabase();

    AssetDatabase(const AssetDatabase&)            = delete;
    AssetDatabase& operator=(const AssetDatabase&) = delete;

    // Synchronous load — blocks the calling thread (editor / tool only).
    template<typename T>
    [[nodiscard]] AssetResult<TypedHandle<T>>
    load_sync(const AssetPath& cooked_path);

    // Async load — returns immediately; state is Pending until tick().
    template<typename T>
    [[nodiscard]] TypedHandle<T> load_async(const AssetPath& cooked_path);

    // Access resident data — nullptr if not Ready.
    template<typename T>
    [[nodiscard]] const T* get(TypedHandle<T> handle) const noexcept;

    void retain(AssetHandle h);
    void release(AssetHandle h);

    // Invalidate handle + re-queue load (called by FSWatcher on hot-reload).
    void invalidate(const AssetPath& cooked_path);

    using ReadyCallback = std::function<void(AssetHandle)>;
    void on_ready(ReadyCallback cb) { ready_cb_ = std::move(cb); }

    // Drive completed async IO; fire ReadyCallbacks. Call once per frame.
    void tick();

    static AssetLoaderRegistry make_default_registry();

private:
    struct Slot {
        AssetMetadata meta;
        LoadedAsset   asset;   // RAII-owning; destroy called automatically
    };

    uint32_t    alloc_slot();
    void        free_slot(uint32_t idx);
    AssetHandle register_or_find(const AssetPath& cooked_path, AssetType type);
    void        enqueue_load(AssetHandle h);
    void        loader_thread_fn();

    /// Null when `AssetDatabaseModHarnessTag` construction path is used.
    render::hal::VulkanDevice*            device_{nullptr};
    vfs::VirtualFilesystem&                 vfs_;
    AssetLoaderRegistry                     loaders_;

    mutable std::mutex                      mutex_;
    std::vector<Slot>                       slots_;
    std::vector<uint32_t>                   free_list_;
    std::unordered_map<AssetPath, uint32_t> path_to_index_;

    struct LoadRequest { AssetHandle handle; AssetPath cooked_path; };
    std::vector<LoadRequest> pending_queue_;
    std::vector<LoadRequest> completed_queue_;
    mutable std::mutex       queue_mutex_;

    // Long-lived async-load worker. Managed by engine/jobs/ (NN #10);
    // destruction order: set stop_loader_ → loader_thread_.join() runs
    // in ~ReservedWorker. See audit-2026-04-20 (P2-1).
    std::atomic<bool>        stop_loader_{false};
    gw::jobs::ReservedWorker loader_thread_;

    ReadyCallback            ready_cb_;
};

// ---------------------------------------------------------------------------
template<typename T>
AssetResult<TypedHandle<T>>
AssetDatabase::load_sync(const AssetPath& cooked_path) {
    AssetHandle h = register_or_find(cooked_path,
                                      static_cast<AssetType>(T::kAssetTypeTag));

    std::vector<uint8_t> raw;
    auto read_res = vfs_.read(cooked_path, raw);
    if (!read_res) {
        std::lock_guard lock{mutex_};
        slots_[h.index()].meta.state = AssetState::Error;
        return std::unexpected(read_res.error());
    }

    const auto* loader = loaders_.find(static_cast<AssetType>(T::kAssetTypeTag));
    if (!loader)
        return std::unexpected(AssetError{AssetErrorCode::InvalidArgument,
                                          "no loader for asset type"});

    if (device_ == nullptr) {
        return std::unexpected(AssetError{AssetErrorCode::InvalidArgument,
                                          "AssetDatabase: no GPU device (harness mode)"});
    }
    auto result = loader->load(*device_, raw);
    if (!result) {
        std::lock_guard lock{mutex_};
        slots_[h.index()].meta.state = AssetState::Error;
        return std::unexpected(result.error());
    }

    {
        std::lock_guard lock{mutex_};
        auto& slot       = slots_[h.index()];
        slot.asset       = std::move(*result);
        slot.meta.state  = AssetState::Ready;
    }

    if (ready_cb_) ready_cb_(h);
    return TypedHandle<T>{h};
}

template<typename T>
TypedHandle<T> AssetDatabase::load_async(const AssetPath& cooked_path) {
    if (device_ == nullptr) {
        return {};
    }
    AssetHandle h = register_or_find(cooked_path,
                                      static_cast<AssetType>(T::kAssetTypeTag));
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
    return static_cast<const T*>(slot.asset.data);
}

}  // namespace gw::assets
