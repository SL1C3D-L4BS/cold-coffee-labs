#include "engine/memory/pool_allocator.hpp"

#include <algorithm>
#include <stdexcept>

namespace gw {
namespace memory {

PoolAllocator::PoolAllocator(std::size_t element_size, std::size_t element_count)
    : element_size_(element_size < sizeof(void*) ? sizeof(void*) : element_size),
      element_count_(element_count),
      free_count_(element_count) {
    if (element_count_ == 0) {
        throw std::invalid_argument("PoolAllocator element_count must be > 0");
    }
    storage_ = std::make_unique<std::byte[]>(element_size_ * element_count_);
    free_list_.reserve(element_count_);
    for (std::size_t i = 0; i < element_count_; ++i) {
        free_list_.push_back(storage_.get() + i * element_size_);
    }
}

void* PoolAllocator::allocate() noexcept {
    if (free_list_.empty()) {
        return nullptr;
    }
    void* ptr = free_list_.back();
    free_list_.pop_back();
    --free_count_;
    return ptr;
}

void PoolAllocator::deallocate(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }
    const auto begin = storage_.get();
    const auto end = storage_.get() + element_size_ * element_count_;
    auto* byte_ptr = static_cast<std::byte*>(ptr);
    if (byte_ptr < begin || byte_ptr >= end) {
        return;
    }
    free_list_.push_back(ptr);
    ++free_count_;
}

}  // namespace memory
}  // namespace gw
