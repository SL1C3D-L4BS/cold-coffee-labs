#pragma once
// engine/console/history.hpp — fixed-size ring of console in/out lines.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gw::console {

class History {
public:
    explicit History(std::size_t capacity = 256) : buf_(capacity), cap_(capacity) {}

    void push(std::string_view line) {
        if (cap_ == 0) return;
        buf_[tail_] = std::string{line};
        tail_ = (tail_ + 1) % cap_;
        if (size_ < cap_) ++size_;
        else head_ = (head_ + 1) % cap_;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return cap_; }

    // Indexed from oldest (0) to newest (size()-1).
    [[nodiscard]] const std::string& at(std::size_t i) const {
        return buf_[(head_ + i) % cap_];
    }

    void clear() noexcept {
        head_ = tail_ = size_ = 0;
    }

    // Serialise to newline-joined text (for session persistence).
    [[nodiscard]] std::string to_text() const {
        std::string s;
        for (std::size_t i = 0; i < size_; ++i) {
            s += at(i);
            s.push_back('\n');
        }
        return s;
    }
    void from_text(std::string_view text) {
        clear();
        std::size_t i = 0;
        while (i < text.size()) {
            auto nl = text.find('\n', i);
            auto line = text.substr(i, (nl == std::string_view::npos ? text.size() : nl) - i);
            if (!line.empty()) push(line);
            if (nl == std::string_view::npos) break;
            i = nl + 1;
        }
    }

private:
    std::vector<std::string> buf_{};
    std::size_t              cap_{0};
    std::size_t              head_{0};
    std::size_t              tail_{0};
    std::size_t              size_{0};
};

} // namespace gw::console
