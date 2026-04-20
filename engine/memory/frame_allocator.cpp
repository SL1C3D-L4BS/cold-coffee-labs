#include "engine/memory/frame_allocator.hpp"

#include <bit>

namespace gw {
namespace memory {

FrameAllocator::FrameAllocator(std::size_t bytes)
    : buffer_(std::make_unique<std::byte[]>(bytes)),
      capacity_(bytes),
      offset_(0) {}

void* FrameAllocator::allocate(std::size_t bytes, std::size_t alignment) noexcept {
    if (bytes == 0 || alignment == 0 || !std::has_single_bit(alignment)) {
        return nullptr;
    }
    const std::size_t aligned = (offset_ + alignment - 1) & ~(alignment - 1);
    if (aligned + bytes > capacity_) {
        return nullptr;
    }
    void* ptr = buffer_.get() + aligned;
    offset_ = aligned + bytes;
    return ptr;
}

void FrameAllocator::reset() noexcept {
    offset_ = 0;
}

}  // namespace memory
}  // namespace gw
