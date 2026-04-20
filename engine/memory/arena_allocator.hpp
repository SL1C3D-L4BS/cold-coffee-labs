#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace gw {
namespace memory {

class ArenaAllocator final {
public:
    explicit ArenaAllocator(std::size_t block_size);
    ~ArenaAllocator() = default;

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t));
    void reset() noexcept;

    [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }
    [[nodiscard]] std::size_t block_count() const noexcept { return blocks_.size(); }

private:
    struct Block {
        std::unique_ptr<std::byte[]> data;
        std::size_t used{0};
    };

    [[nodiscard]] Block& current_block();
    void add_block();

    std::size_t block_size_{0};
    std::vector<Block> blocks_{};
};

}  // namespace memory
}  // namespace gw
