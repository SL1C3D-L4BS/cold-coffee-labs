#pragma once

#include <string>

namespace gw::platform {

class DynamicLibrary final {
public:
    DynamicLibrary() = default;
    explicit DynamicLibrary(const std::string& path);
    ~DynamicLibrary();

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    [[nodiscard]] bool open(const std::string& path);
    void close() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] void* find_symbol(const char* name) const noexcept;

private:
    void* handle_{nullptr};
};

}  // namespace gw::platform
