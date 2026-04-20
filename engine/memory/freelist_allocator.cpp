#include "engine/memory/freelist_allocator.hpp"

#include <stdexcept>

namespace gw::memory {

FreelistAllocator::FreelistAllocator(std::size_t capacity_bytes)
    : storage_(std::make_unique<std::byte[]>(capacity_bytes)),
      capacity_bytes_(capacity_bytes) {
    if (capacity_bytes_ == 0) {
        throw std::invalid_argument("FreelistAllocator capacity must be > 0");
    }
    free_blocks_.emplace(0, capacity_bytes_);
}

void* FreelistAllocator::allocate(std::size_t bytes) {
    if (bytes == 0) {
        return nullptr;
    }
    for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
        const std::size_t offset = it->first;
        const std::size_t size = it->second;
        if (size < bytes) {
            continue;
        }
        free_blocks_.erase(it);
        if (size > bytes) {
            free_blocks_.emplace(offset + bytes, size - bytes);
        }
        return storage_.get() + offset;
    }
    return nullptr;
}

void FreelistAllocator::deallocate(void* ptr, std::size_t bytes) noexcept {
    if (ptr == nullptr || bytes == 0) {
        return;
    }
    const std::size_t offset = static_cast<std::size_t>(static_cast<std::byte*>(ptr) - storage_.get());
    if (offset >= capacity_bytes_) {
        return;
    }
    free_blocks_[offset] = bytes;
}

}  // namespace gw::memory
