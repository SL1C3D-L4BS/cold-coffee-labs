#include "engine/platform/window.hpp"
#include <utility>

namespace gw::platform {

Window::Window(WindowDesc desc)
    : width_(desc.width),
      height_(desc.height),
      title_(std::move(desc.title)) {}

Window::~Window() {
    release();
}

Window::Window(Window&& other) noexcept
    : handle_(other.handle_),
      width_(other.width_),
      height_(other.height_),
      title_(std::move(other.title_)) {
    other.handle_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    release();
    handle_ = other.handle_;
    width_ = other.width_;
    height_ = other.height_;
    title_ = std::move(other.title_);
    other.handle_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
    return *this;
}

bool Window::should_close() const noexcept {
    return false;
}

void Window::poll_events() const noexcept {
}

void Window::release() noexcept {
    handle_ = nullptr;
}

}  // namespace gw::platform
