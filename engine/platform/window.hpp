#pragma once

#include <cstdint>
#include <string>

namespace gw {
namespace platform {

struct WindowDesc {
    std::string title{"Greywater"};
    int width{1280};
    int height{720};
    bool resizable{true};
};

class Window final {
public:
    explicit Window(WindowDesc desc);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    [[nodiscard]] bool should_close() const noexcept;
    void poll_events() const noexcept;
    [[nodiscard]] void* native_handle() const noexcept { return handle_; }
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

private:
    void release() noexcept;

    void* handle_{nullptr};
    int width_{0};
    int height_{0};
    std::string title_{};
};

}  // namespace platform
}  // namespace gw
