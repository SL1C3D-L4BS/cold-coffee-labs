#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace gw::memory {

class PoolAllocator final {
public:
    PoolAllocator(std::size_t element_size, std::size_t element_count);
    ~PoolAllocator() = default;

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    [[nodiscard]] void* allocate() noexcept;
    void deallocate(void* ptr) noexcept;

    [[nodiscard]] std::size_t free_count() const noexcept { return free_count_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return element_count_; }

private:
    std::unique_ptr<std::byte[]> storage_{};
    std::vector<void*> free_list_{};
    std::size_t element_size_{0};
    std::size_t element_count_{0};
    std::size_t free_count_{0};
};

}  // namespace gw::memory
