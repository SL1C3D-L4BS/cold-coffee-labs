#include "asset_db.hpp"
#include "mesh_asset.hpp"
#include "texture_asset.hpp"
#include <algorithm>

namespace gw::assets {

AssetDatabase::AssetDatabase(render::hal::VulkanDevice& device,
                              vfs::VirtualFilesystem&    vfs)
    : device_(device)
    , vfs_(vfs)
{
    slots_.reserve(4096);
    loaders_ = make_default_registry();

    // Start background loader thread.
    loader_thread_ = std::thread(&AssetDatabase::loader_thread_fn, this);
}

AssetDatabase::~AssetDatabase() {
    stop_loader_ = true;
    loader_thread_.join();

    // Destroy all resident assets.
    for (auto& slot : slots_) {
        if (slot.data_ptr && slot.meta.state == AssetState::Ready) {
            const auto* entry = loaders_.find(slot.meta.type);
            if (entry) entry->destroy(slot.data_ptr);
            slot.data_ptr = nullptr;
        }
    }
}

AssetLoaderRegistry AssetDatabase::make_default_registry() {
    AssetLoaderRegistry reg;
    reg.register_loader<MeshAsset>();
    reg.register_loader<TextureAsset>();
    return reg;
}

uint32_t AssetDatabase::alloc_slot() {
    if (!free_list_.empty()) {
        const uint32_t idx = free_list_.back();
        free_list_.pop_back();
        return idx;
    }
    slots_.emplace_back();
    return static_cast<uint32_t>(slots_.size() - 1u);
}

void AssetDatabase::free_slot(uint32_t idx) {
    auto& slot     = slots_[idx];
    slot.data_ptr  = nullptr;
    slot.meta.state= AssetState::Unloaded;
    ++slot.meta.generation;
    free_list_.push_back(idx);
}

AssetHandle AssetDatabase::register_or_find(const AssetPath& cooked_path,
                                             AssetType type)
{
    std::lock_guard lock{mutex_};
    auto it = path_to_index_.find(cooked_path);
    if (it != path_to_index_.end()) {
        const auto& s = slots_[it->second];
        return AssetHandle::make(it->second, s.meta.generation,
                                 static_cast<uint16_t>(type));
    }
    const uint32_t idx = alloc_slot();
    auto& slot       = slots_[idx];
    slot.meta.cooked_path = cooked_path;
    slot.meta.type        = type;
    slot.meta.state       = AssetState::Unloaded;
    path_to_index_[cooked_path] = idx;
    return AssetHandle::make(idx, slot.meta.generation,
                             static_cast<uint16_t>(type));
}

void AssetDatabase::enqueue_load(AssetHandle h) {
    {
        std::lock_guard lock{mutex_};
        slots_[h.index()].meta.state = AssetState::Pending;
    }
    std::lock_guard ql{queue_mutex_};
    pending_queue_.push_back({h, slots_[h.index()].meta.cooked_path});
}

void AssetDatabase::loader_thread_fn() {
    while (!stop_loader_) {
        LoadRequest req;
        {
            std::lock_guard ql{queue_mutex_};
            if (pending_queue_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            req = pending_queue_.front();
            pending_queue_.erase(pending_queue_.begin());
        }

        // Read from VFS.
        std::vector<uint8_t> raw;
        auto read_res = vfs_.read(req.cooked_path, raw);

        AssetType type;
        {
            std::lock_guard lock{mutex_};
            type = slots_[req.handle.index()].meta.type;
        }

        if (!read_res) {
            std::lock_guard lock{mutex_};
            slots_[req.handle.index()].meta.state = AssetState::Error;
            continue;
        }

        const auto* loader = loaders_.find(type);
        if (!loader) {
            std::lock_guard lock{mutex_};
            slots_[req.handle.index()].meta.state = AssetState::Error;
            continue;
        }

        auto result = loader->load(device_, raw);
        {
            std::lock_guard lock{mutex_};
            auto& slot = slots_[req.handle.index()];
            if (result) {
                slot.data_ptr  = *result;
                slot.meta.state= AssetState::Ready;
            } else {
                slot.meta.state= AssetState::Error;
            }
        }

        std::lock_guard ql{queue_mutex_};
        completed_queue_.push_back(req);
    }
}

void AssetDatabase::tick() {
    std::vector<LoadRequest> completed;
    {
        std::lock_guard ql{queue_mutex_};
        completed.swap(completed_queue_);
    }
    if (ready_cb_) {
        for (const auto& req : completed) {
            AssetState state;
            {
                std::lock_guard lock{mutex_};
                state = slots_[req.handle.index()].meta.state;
            }
            if (state == AssetState::Ready) {
                ready_cb_(req.handle);
            }
        }
    }
}

void AssetDatabase::retain(AssetHandle h) {
    std::lock_guard lock{mutex_};
    if (h.index() < slots_.size())
        ++slots_[h.index()].meta.ref_count;
}

void AssetDatabase::release(AssetHandle h) {
    std::lock_guard lock{mutex_};
    if (h.index() < slots_.size())
        --slots_[h.index()].meta.ref_count;
}

void AssetDatabase::invalidate(const AssetPath& cooked_path) {
    uint32_t idx = UINT32_MAX;
    {
        std::lock_guard lock{mutex_};
        auto it = path_to_index_.find(cooked_path);
        if (it == path_to_index_.end()) return;
        idx = it->second;
        auto& slot = slots_[idx];
        if (slot.data_ptr) {
            const auto* entry = loaders_.find(slot.meta.type);
            if (entry) entry->destroy(slot.data_ptr);
            slot.data_ptr = nullptr;
        }
        slot.meta.state = AssetState::Pending;
    }
    std::lock_guard ql{queue_mutex_};
    pending_queue_.push_back({AssetHandle::make(idx, slots_[idx].meta.generation,
                              static_cast<uint16_t>(slots_[idx].meta.type)),
                              cooked_path});
}

} // namespace gw::assets
