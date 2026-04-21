#pragma once
// engine/assets/asset_error.hpp
// Typed error for all asset subsystem APIs.
// Stack-allocated, no heap, safe to store in std::expected / gw::core::Result.

#include <cstdint>
#include <cstring>
#include <expected>
#include <variant>

namespace gw::assets {

enum class AssetErrorCode : uint16_t {
    Ok = 0,
    FileNotFound,
    InvalidMagic,
    UnsupportedVersion,
    CorruptData,
    GpuUploadFailed,
    CookFailed,
    DependencyMissing,
    OutOfMemory,
    InvalidArgument,
    NotReady,
};

struct AssetError {
    AssetErrorCode code    = AssetErrorCode::Ok;
    char           message[126]{};

    AssetError() = default;

    AssetError(AssetErrorCode c, const char* msg) noexcept : code(c) {
        if (msg) {
            const auto len  = std::strlen(msg);
            const auto copy = len < sizeof(message) - 1u ? len : sizeof(message) - 1u;
            std::memcpy(message, msg, copy);
            message[copy] = '\0';
        }
    }
};

// C++23 std::expected aliases used throughout Phase 6+.
template<typename T>
using AssetResult = std::expected<T, AssetError>;

using AssetOk = AssetResult<std::monostate>;

// Convenience factories.
template<typename T>
[[nodiscard]] AssetResult<T> asset_ok(T val) {
    return AssetResult<T>{std::move(val)};
}

[[nodiscard]] inline AssetOk asset_ok() {
    return AssetOk{std::monostate{}};
}

[[nodiscard]] inline std::unexpected<AssetError>
asset_err(AssetErrorCode code, const char* msg = "") {
    return std::unexpected(AssetError{code, msg});
}

} // namespace gw::assets
