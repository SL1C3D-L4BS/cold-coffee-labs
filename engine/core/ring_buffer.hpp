#pragma once

#include <optional>
#include <vector>

namespace gw {
namespace core {

template <typename T>
class RingBuffer final {
public:
    explicit RingBuffer(std::size_t capacity)
        : buffer_(capacity),
          capacity_(capacity) {}

    [[nodiscard]] bool push(const T& value) {
        if (size_ == capacity_) {
            return false;
        }
        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % capacity_;
        ++size_;
        return true;
    }

    [[nodiscard]] std::optional<T> pop() {
        if (size_ == 0) {
            return std::nullopt;
        }
        const T value = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        --size_;
        return value;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

private:
    std::vector<T> buffer_;
    std::size_t capacity_{0};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
};

}  // namespace core
}  // namespace gw
