#include "engine/memory/arena_allocator.hpp"

#include <bit>
#include <stdexcept>

namespace gw::memory {

ArenaAllocator::ArenaAllocator(std::size_t block_size)
    : block_size_(block_size) {
    if (block_size_ == 0) {
        throw std::invalid_argument("ArenaAllocator block_size must be > 0");
    }
    add_block();
}

void* ArenaAllocator::allocate(std::size_t bytes, std::size_t alignment) {
    if (bytes == 0 || alignment == 0 || !std::has_single_bit(alignment)) {
        return nullptr;
    }
    Block* block = &current_block();
    std::size_t aligned = (block->used + alignment - 1) & ~(alignment - 1);
    if (aligned + bytes > block_size_) {
        add_block();
        block = &current_block();
        aligned = 0;
    }
    void* ptr = block->data.get() + aligned;
    block->used = aligned + bytes;
    return ptr;
}

void ArenaAllocator::reset() noexcept {
    for (auto& block : blocks_) {
        block.used = 0;
    }
}

ArenaAllocator::Block& ArenaAllocator::current_block() {
    return blocks_.back();
}

void ArenaAllocator::add_block() {
    Block block{};
    block.data = std::make_unique<std::byte[]>(block_size_);
    block.used = 0;
    blocks_.push_back(std::move(block));
}

}  // namespace gw::memory
