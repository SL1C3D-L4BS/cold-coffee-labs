#pragma once
// engine/assets/asset_handle.hpp
// AssetHandle — 64-bit, trivially copyable, ABA-safe typed handle.
//
// Layout:  [63:48] type_tag (uint16)
//          [47:32] generation (uint16)
//          [31:0]  slot index (uint32)
//
// Typed wrapper TypedHandle<T> is zero-cost at runtime; it carries the type
// tag as a compile-time constant and enables static_assert checks at call
// sites without virtual dispatch or RTTI.

#include "asset_types.hpp"
#include <cstdint>
#include <functional>

namespace gw::assets {

struct AssetHandle {
    uint64_t bits = 0;

    static constexpr AssetHandle null() noexcept { return {}; }

    [[nodiscard]] bool     valid()      const noexcept { return bits != 0; }
    [[nodiscard]] uint32_t index()      const noexcept { return static_cast<uint32_t>(bits); }
    [[nodiscard]] uint16_t generation() const noexcept { return static_cast<uint16_t>(bits >> 32u); }
    [[nodiscard]] uint16_t type_tag()   const noexcept { return static_cast<uint16_t>(bits >> 48u); }

    static AssetHandle make(uint32_t idx, uint16_t gen, uint16_t tag) noexcept {
        return AssetHandle{
            (static_cast<uint64_t>(tag) << 48u) |
            (static_cast<uint64_t>(gen) << 32u) |
            static_cast<uint64_t>(idx)
        };
    }

    bool operator==(const AssetHandle& o) const noexcept { return bits == o.bits; }
    bool operator!=(const AssetHandle& o) const noexcept { return bits != o.bits; }
};

// Typed handle — narrows the type tag to a compile-time constant.
// T must declare: static constexpr uint16_t kAssetTypeTag.
template<typename T>
struct TypedHandle : AssetHandle {
    static constexpr uint16_t kTypeTag = T::kAssetTypeTag;

    TypedHandle() noexcept : AssetHandle{} {}

    explicit TypedHandle(AssetHandle h) noexcept : AssetHandle{h} {}

    // Construction helper — enforces the correct type tag.
    static TypedHandle make(uint32_t idx, uint16_t gen) noexcept {
        return TypedHandle{AssetHandle::make(idx, gen, kTypeTag)};
    }
};

} // namespace gw::assets

// std::hash for AssetHandle — allows use in unordered containers.
namespace std {
template<>
struct hash<gw::assets::AssetHandle> {
    std::size_t operator()(const gw::assets::AssetHandle& h) const noexcept {
        return std::hash<uint64_t>{}(h.bits);
    }
};
} // namespace std
