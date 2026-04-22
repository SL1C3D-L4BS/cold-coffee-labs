#pragma once
// engine/ecs/component_registry.hpp
// ComponentRegistry — runtime per-type-info table for ECS components.
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.4.
//
// Each component type registered with the registry gets:
//   - A ComponentTypeId (small integer, stable within a process).
//   - Size, alignment, trivially-copyable flag.
//   - Lifecycle function pointers (copy-ctor / move-ctor / destructor) populated
//     only when the type is NOT trivially copyable+destructible.
//   - A stable FNV1a hash of the fully-qualified type name (the *portable*
//     identifier used for serialization).
//
// The registry is owned by gw::ecs::World; components are registered implicitly
// on first lookup via id_of<T>().

#include "entity.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace gw::core {
struct TypeInfo;   // forward declare from engine/core/field_reflection.hpp
} // namespace gw::core

namespace gw {
namespace ecs {

struct ComponentTypeInfo {
    ComponentTypeId  id                 = kInvalidComponentTypeId;
    std::size_t      size               = 0;
    std::size_t      alignment          = 0;
    bool             trivially_copyable = false;

    // FNV1a-64 of a compile-time type name string (see gw_type_name<T>() in
    // detail/). Stable across the same source build on the same compiler; the
    // portable identifier used at serialization boundaries.
    std::uint64_t    stable_hash        = 0;

    // Lifecycle hooks. Null when the type is trivially copyable / destructible.
    void (*copy_construct)(void* dst, const void* src) = nullptr;
    void (*move_construct)(void* dst, void* src) noexcept = nullptr;
    void (*destruct)(void* p) noexcept = nullptr;

    // Reflection pointer (populated via ADL on describe(T*); nullptr if the type
    // has no GW_REFLECT).
    const gw::core::TypeInfo* reflection = nullptr;

    // Stable debug name (static storage duration; never freed).
    std::string_view debug_name;
};

class ComponentRegistry {
public:
    ComponentRegistry();
    ~ComponentRegistry();

    ComponentRegistry(const ComponentRegistry&)            = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;
    ComponentRegistry(ComponentRegistry&&) noexcept;
    ComponentRegistry& operator=(ComponentRegistry&&) noexcept;

    // Lookup-or-register. First call registers T with this registry; subsequent
    // calls are O(1) via a per-T static cache.
    template <typename T>
    [[nodiscard]] ComponentTypeId id_of();

    template <typename T>
    [[nodiscard]] ComponentTypeId id_of_or_invalid() const noexcept;

    [[nodiscard]] const ComponentTypeInfo& info(ComponentTypeId id) const;
    [[nodiscard]] std::size_t component_count() const noexcept { return infos_.size(); }

    [[nodiscard]] std::optional<ComponentTypeId>
    lookup_by_hash(std::uint64_t stable_hash) const noexcept;

    // Internal: used by id_of<T>() and by the friend helpers in the detail
    // namespace. Not intended for direct use.
    [[nodiscard]] ComponentTypeId register_raw(const ComponentTypeInfo& info);

private:
    std::vector<ComponentTypeInfo>                       infos_;
    std::unordered_map<std::uint64_t, ComponentTypeId>   hash_to_id_;

    // Per-process stable_hash → ComponentTypeId cache that bridges
    // per-type-static-cached ids to a specific registry instance.
    // Keys are stable_hash values; populated by id_of<T>() and register_raw.
    std::unordered_map<std::uint64_t, ComponentTypeId>   lookup_cache_;
};

// ---------------------------------------------------------------------------
// Implementation: template surface.
// Lives in the header so callers don't need a separate instantiation step.
// ---------------------------------------------------------------------------

namespace detail {

// FNV1a-64 of a null-terminated C string. constexpr-able; used at compile time
// via a type-name helper.
[[nodiscard]] constexpr std::uint64_t fnv1a_64(std::string_view s) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) {
        h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        h *= 0x100000001b3ULL;
    }
    return h;
}

// Compile-time fully-qualified type name of T, lifted out of the compiler's
// __PRETTY_FUNCTION__ / __FUNCSIG__ macro. Returns a std::string_view that
// points into the containing function's static read-only data — stable for
// the life of the program.
template <typename T>
[[nodiscard]] constexpr std::string_view gw_type_name() noexcept {
#if defined(__clang__) || defined(__GNUC__)
    // "constexpr std::string_view gw::ecs::detail::gw_type_name() [T = TypeName]"
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
    constexpr std::string_view marker = "T = ";
    constexpr auto begin = sig.find(marker) + marker.size();
    constexpr auto end   = sig.rfind(']');
    return sig.substr(begin, end - begin);
#elif defined(_MSC_VER)
    constexpr std::string_view sig = __FUNCSIG__;
    constexpr std::string_view marker = "gw_type_name<";
    constexpr auto begin = sig.find(marker) + marker.size();
    constexpr auto end   = sig.rfind(">(");
    return sig.substr(begin, end - begin);
#else
#  error "gw_type_name not supported on this compiler"
#endif
}

// Detect via ADL whether a free function `describe(T*)` returning
// `const gw::core::TypeInfo&` exists. Populated by the GW_REFLECT macro
// (see editor/reflect/reflect.hpp); silently nullptr for components without it.
template <typename T>
concept HasDescribe = requires (T* p) {
    { describe(p) } -> std::convertible_to<const gw::core::TypeInfo&>;
};

template <typename T>
[[nodiscard]] inline const gw::core::TypeInfo* reflection_pointer() noexcept {
    if constexpr (HasDescribe<T>) {
        return &describe(static_cast<T*>(nullptr));
    } else {
        return nullptr;
    }
}

template <typename T>
[[nodiscard]] inline ComponentTypeInfo make_info_for() noexcept {
    constexpr auto name = gw_type_name<T>();
    ComponentTypeInfo info{};
    info.size               = sizeof(T);
    info.alignment          = alignof(T);
    info.trivially_copyable = std::is_trivially_copyable_v<T> &&
                              std::is_trivially_destructible_v<T>;
    info.stable_hash        = fnv1a_64(name);
    info.debug_name         = name;
    info.reflection         = reflection_pointer<T>();

    if constexpr (std::is_copy_constructible_v<T>) {
        info.copy_construct = [](void* dst, const void* src) {
            ::new (dst) T(*static_cast<const T*>(src));
        };
    }
    if constexpr (std::is_move_constructible_v<T>) {
        info.move_construct = [](void* dst, void* src) noexcept {
            ::new (dst) T(std::move(*static_cast<T*>(src)));
        };
    }
    info.destruct = [](void* p) noexcept {
        static_cast<T*>(p)->~T();
    };

    return info;
}

} // namespace detail

template <typename T>
inline ComponentTypeId ComponentRegistry::id_of() {
    static constexpr std::uint64_t hash =
        detail::fnv1a_64(detail::gw_type_name<T>());
    auto it = lookup_cache_.find(hash);
    if (it != lookup_cache_.end()) return it->second;
    const auto id = register_raw(detail::make_info_for<T>());
    lookup_cache_.emplace(hash, id);
    return id;
}

template <typename T>
inline ComponentTypeId ComponentRegistry::id_of_or_invalid() const noexcept {
    static constexpr std::uint64_t hash =
        detail::fnv1a_64(detail::gw_type_name<T>());
    auto it = hash_to_id_.find(hash);
    return (it == hash_to_id_.end()) ? kInvalidComponentTypeId : it->second;
}

} // namespace ecs
} // namespace gw
