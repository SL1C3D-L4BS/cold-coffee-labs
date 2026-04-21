#pragma once
// engine/core/type_registry.hpp
// Stable compile-time type IDs based on FNV1a hash of the type name.
// Uses __PRETTY_FUNCTION__ / __FUNCSIG__ which is consistent within a TU and
// does NOT rely on RTTI or typeid — safe across DLL/shared-lib boundaries.
//
// Deliberately avoids <string_view> to prevent the MSVC STL 14.44 + clang-cl
// circular include issue where __msvc_string_view.hpp (line ~550) instantiates
// basic_ostream before <ostream> is fully processed.
//
// Spec ref: Phase 7 §3 — "fnv1a of type name — stable, no RTTI".

#include <cstdint>
#include <cstddef>

namespace gw::core {

// ---------------------------------------------------------------------------
// FNV1a 32-bit compile-time hash
// ---------------------------------------------------------------------------
namespace detail {

// Constexpr strlen — avoids pulling in <cstring>.
[[nodiscard]] constexpr std::size_t cstrlen(const char* s) noexcept {
    std::size_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

[[nodiscard]] constexpr uint32_t fnv1a_32(const char* s, std::size_t n) noexcept {
    uint32_t h = 0x811c9dc5u;
    for (std::size_t i = 0; i < n; ++i) {
        h ^= static_cast<uint32_t>(static_cast<unsigned char>(s[i]));
        h *= 0x01000193u;
    }
    return h;
}

// Whole-string overload (includes null terminator if N is array size).
template<std::size_t N>
[[nodiscard]] constexpr uint32_t fnv1a_32(const char (&s)[N]) noexcept {
    return fnv1a_32(s, N - 1);  // exclude null terminator
}

// Returns the compiler-decorated type string for T.
// The full __FUNCSIG__ / __PRETTY_FUNCTION__ is stable within a toolchain
// version.  We hash the whole thing; identity, not readability, is the goal.
template<typename T>
[[nodiscard]] constexpr const char* decorated_type_name() noexcept {
#if defined(_MSC_VER)
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

}  // namespace detail

// ---------------------------------------------------------------------------
// TypeRegistry — stateless, all computation at compile time.
// ---------------------------------------------------------------------------
struct TypeRegistry {
    template<typename T>
    [[nodiscard]] static constexpr uint32_t get_type_id() noexcept {
        const char* s = detail::decorated_type_name<T>();
        return detail::fnv1a_32(s, detail::cstrlen(s));
    }

    // Returns the raw decorated type name string.  Lifetime: static (literal).
    template<typename T>
    [[nodiscard]] static constexpr const char* get_type_name() noexcept {
        return detail::decorated_type_name<T>();
    }
};

// Convenience free function — shorter call site.
template<typename T>
[[nodiscard]] constexpr uint32_t type_id() noexcept {
    return TypeRegistry::get_type_id<T>();
}

// Stable FNV1a of a null-terminated string literal — used by the reflect macro.
[[nodiscard]] constexpr uint32_t fnv1a(const char* s) noexcept {
    return detail::fnv1a_32(s, detail::cstrlen(s));
}

// Array overload: avoids calling cstrlen on a literal (length known at compile time).
template<std::size_t N>
[[nodiscard]] constexpr uint32_t fnv1a(const char (&s)[N]) noexcept {
    return detail::fnv1a_32(s, N - 1);
}

}  // namespace gw::core
