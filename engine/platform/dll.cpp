#include "engine/platform/dll.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace gw::platform {

DynamicLibrary::DynamicLibrary(const std::string& path) {
    static_cast<void>(open(path));
}

DynamicLibrary::~DynamicLibrary() {
    close();
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = nullptr;
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    close();
    handle_ = other.handle_;
    other.handle_ = nullptr;
    return *this;
}

bool DynamicLibrary::open(const std::string& path) {
    close();
#if defined(_WIN32)
    handle_ = static_cast<void*>(::LoadLibraryA(path.c_str()));
#else
    handle_ = ::dlopen(path.c_str(), RTLD_NOW);
#endif
    return handle_ != nullptr;
}

void DynamicLibrary::close() noexcept {
    if (handle_ == nullptr) {
        return;
    }
#if defined(_WIN32)
    ::FreeLibrary(static_cast<HMODULE>(handle_));
#else
    ::dlclose(handle_);
#endif
    handle_ = nullptr;
}

bool DynamicLibrary::is_open() const noexcept {
    return handle_ != nullptr;
}

void* DynamicLibrary::find_symbol(const char* name) const noexcept {
    if (handle_ == nullptr) {
        return nullptr;
    }
#if defined(_WIN32)
    return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
    return ::dlsym(handle_, name);
#endif
}

}  // namespace gw::platform
