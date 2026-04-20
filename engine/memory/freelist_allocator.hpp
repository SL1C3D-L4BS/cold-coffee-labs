#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>

namespace gw::memory {

class FreelistAllocator final {
public:
    explicit FreelistAllocator(std::size_t capacity_bytes);
    ~FreelistAllocator() = default;

    FreelistAllocator(const FreelistAllocator&) = delete;
    FreelistAllocator& operator=(const FreelistAllocator&) = delete;

    [[nodiscard]] void* allocate(std::size_t bytes);
    void deallocate(void* ptr, std::size_t bytes) noexcept;

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_bytes_; }

private:
    std::unique_ptr<std::byte[]> storage_{};
    std::map<std::size_t, std::size_t> free_blocks_{};
    std::size_t capacity_bytes_{0};
};

}  // namespace gw::memory
