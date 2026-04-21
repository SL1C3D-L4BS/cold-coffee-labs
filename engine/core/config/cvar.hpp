#pragma once
// engine/core/config/cvar.hpp — typed CVar handle + flags (ADR-0024).
//
// A CVar<T> is a tiny value: a 32-bit ID into the global CVarRegistry.
// Reads go through the registry's snapshot; writes funnel through the
// registry's mutex + version counter.
//
// Supported T:
//   bool, int32_t, int64_t, float, double, std::string,
//   enum class derived from int32_t (via std::underlying_type_t).

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace gw::config {

enum CVarFlags : std::uint32_t {
    kCVarNone             = 0,
    kCVarPersist          = 1u << 0,  // saved to its domain .toml on exit
    kCVarUserFacing       = 1u << 1,  // appears in settings menu
    kCVarDevOnly          = 1u << 2,  // stripped in release unless --allow-console
    kCVarCheat            = 1u << 3,  // flips "CHEATS ENABLED" banner when deviated
    kCVarReadOnly         = 1u << 4,  // writes from console / RML ignored
    kCVarRenderThreadSafe = 1u << 5,  // safe to read on render thread (snapshot-load)
    kCVarRequiresRestart  = 1u << 6,  // note: change persisted, but not applied until restart
    kCVarHotReload        = 1u << 7,  // reload from disk on file-watch trigger
};

// Origin bits on ConfigCVarChanged — matches the enum packed into
// `events_core::ConfigCVarChanged::origin`.
enum CVarOrigin : std::uint32_t {
    kOriginProgrammatic = 0,
    kOriginConsole      = 1u << 0,
    kOriginTomlLoad     = 1u << 1,
    kOriginRmlBinding   = 1u << 2,
    kOriginBld          = 1u << 3,
};

enum class CVarType : std::uint8_t {
    Bool = 0,
    Int32,
    Int64,
    Float,
    Double,
    String,
    Enum,    // stored as int32; value table supplied at registration
};

struct CVarId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const CVarId&) const noexcept = default;
};

// Type traits — map T → CVarType.
template <typename T> struct CVarTypeOf;
template <> struct CVarTypeOf<bool>        { static constexpr CVarType value = CVarType::Bool;   };
template <> struct CVarTypeOf<std::int32_t>{ static constexpr CVarType value = CVarType::Int32;  };
template <> struct CVarTypeOf<std::int64_t>{ static constexpr CVarType value = CVarType::Int64;  };
template <> struct CVarTypeOf<float>       { static constexpr CVarType value = CVarType::Float;  };
template <> struct CVarTypeOf<double>      { static constexpr CVarType value = CVarType::Double; };
template <> struct CVarTypeOf<std::string> { static constexpr CVarType value = CVarType::String; };

// A simple {label, int_value} table for enum CVars. Used by toml_binding
// to round-trip through human-readable names.
struct EnumVariant {
    std::string_view label{};
    std::int32_t     value{0};
};

// -----------------------------------------------------------------------
// CVarRef<T> — thin handle you hold in a system after registration.
// -----------------------------------------------------------------------
template <typename T>
struct CVarRef {
    CVarId id{};
    [[nodiscard]] constexpr bool valid() const noexcept { return id.valid(); }
};

// Initial-value helper used when constructing a CVar at registration.
template <typename T>
struct CVarInit {
    std::string_view toml_path;   // e.g. "audio.master.volume"
    T                default_value{};
    std::uint32_t    flags{kCVarNone};
    // Range hints (optional; used by the settings UI + load-time clamp).
    T                min_value{};
    T                max_value{};
    bool             has_range{false};
    // Description shown in `help` and tooltip.
    std::string_view description{};
};

} // namespace gw::config
