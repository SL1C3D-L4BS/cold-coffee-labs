#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace gw::memory {

class FrameAllocator final {
public:
    explicit FrameAllocator(std::size_t bytes);
    ~FrameAllocator() = default;

    FrameAllocator(const FrameAllocator&) = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
    void reset() noexcept;

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t used() const noexcept { return offset_; }

private:
    std::unique_ptr<std::byte[]> buffer_{};
    std::size_t capacity_{0};
    std::size_t offset_{0};
};

}  // namespace gw::memory
