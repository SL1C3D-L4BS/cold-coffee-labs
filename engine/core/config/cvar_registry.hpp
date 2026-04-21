#pragma once
// engine/core/config/cvar_registry.hpp — typed CVar registry (ADR-0024).
//
// Design:
//   * Every CVar lives in a single registry instance addressed by CVarId.
//   * Readers fetch through the registry's snapshot (lock-free per-read).
//   * Writers take a mutex, bump the value, bump a global version, and
//     publish ConfigCVarChanged on the optional bus pointer.
//
// This registry is intentionally single-threaded for Phase 11: the main
// thread owns it. The "snapshot" machinery is the seam for the render
// thread to pick up non-stale reads without locking in Phase 12+.

#include "engine/core/config/cvar.hpp"
#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::config {

struct CVarEntry {
    std::string   name{};           // fully-qualified: "audio.master.volume"
    std::string   description{};
    CVarType      type{CVarType::Bool};
    std::uint32_t flags{kCVarNone};
    // Storage. We reserve one slot of every scalar so the handle type
    // is independent of the runtime representation.
    bool           v_bool{false};
    std::int32_t   v_i32{0};
    std::int64_t   v_i64{0};
    float          v_f32{0.0f};
    double         v_f64{0.0};
    std::string    v_str{};
    // Default values (used by `reset_to_default`).
    bool           d_bool{false};
    std::int32_t   d_i32{0};
    std::int64_t   d_i64{0};
    float          d_f32{0.0f};
    double         d_f64{0.0};
    std::string    d_str{};
    // Ranges (inclusive). Used by load-time clamp + settings UI.
    bool           has_range{false};
    double         lo{0.0};
    double         hi{0.0};
    // Enum labels.
    std::vector<EnumVariant> enum_variants{};
};

class CVarRegistry {
public:
    CVarRegistry() = default;
    ~CVarRegistry() = default;
    CVarRegistry(const CVarRegistry&) = delete;
    CVarRegistry& operator=(const CVarRegistry&) = delete;

    // Optional bus for ConfigCVarChanged publishing. Set once at boot.
    void set_bus(events::EventBus<events::ConfigCVarChanged>* bus) noexcept {
        bus_ = bus;
    }

    // Registration API. Returns the fresh CVarRef. If a CVar with the
    // same name is already registered, returns the existing id (use
    // `find` + `set_*` to update).
    [[nodiscard]] CVarRef<bool>         register_bool  (const CVarInit<bool>&);
    [[nodiscard]] CVarRef<std::int32_t> register_i32   (const CVarInit<std::int32_t>&);
    [[nodiscard]] CVarRef<std::int64_t> register_i64   (const CVarInit<std::int64_t>&);
    [[nodiscard]] CVarRef<float>        register_f32   (const CVarInit<float>&);
    [[nodiscard]] CVarRef<double>       register_f64   (const CVarInit<double>&);
    [[nodiscard]] CVarRef<std::string>  register_string(const CVarInit<std::string>&);
    // Enum — stored as int32 with a variant table.
    [[nodiscard]] CVarRef<std::int32_t> register_enum  (std::string_view name,
                                                        std::span<const EnumVariant> variants,
                                                        std::int32_t default_value,
                                                        std::uint32_t flags = kCVarNone,
                                                        std::string_view desc = {});

    // Find by name.
    [[nodiscard]] CVarId find(std::string_view name) const noexcept;

    // Typed getters (return nullopt if id invalid or type mismatch).
    [[nodiscard]] std::optional<bool>         get_bool  (CVarId) const noexcept;
    [[nodiscard]] std::optional<std::int32_t> get_i32   (CVarId) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> get_i64   (CVarId) const noexcept;
    [[nodiscard]] std::optional<float>        get_f32   (CVarId) const noexcept;
    [[nodiscard]] std::optional<double>       get_f64   (CVarId) const noexcept;
    [[nodiscard]] std::optional<std::string>  get_string(CVarId) const;

    // Convenience accessors by name — used by the console + RML binder.
    [[nodiscard]] bool          get_bool_or  (std::string_view name, bool        fallback = false) const noexcept;
    [[nodiscard]] std::int32_t  get_i32_or   (std::string_view name, std::int32_t fallback = 0) const noexcept;
    [[nodiscard]] float         get_f32_or   (std::string_view name, float       fallback = 0.0f) const noexcept;
    [[nodiscard]] double        get_f64_or   (std::string_view name, double      fallback = 0.0) const noexcept;
    [[nodiscard]] std::string   get_string_or(std::string_view name, std::string_view fallback = {}) const;

    // Typed setters. Returns false if id invalid, type mismatch, or
    // read-only violation.
    [[nodiscard]] bool set_bool  (CVarId, bool         v, std::uint32_t origin = kOriginProgrammatic);
    [[nodiscard]] bool set_i32   (CVarId, std::int32_t v, std::uint32_t origin = kOriginProgrammatic);
    [[nodiscard]] bool set_i64   (CVarId, std::int64_t v, std::uint32_t origin = kOriginProgrammatic);
    [[nodiscard]] bool set_f32   (CVarId, float        v, std::uint32_t origin = kOriginProgrammatic);
    [[nodiscard]] bool set_f64   (CVarId, double       v, std::uint32_t origin = kOriginProgrammatic);
    [[nodiscard]] bool set_string(CVarId, std::string_view v, std::uint32_t origin = kOriginProgrammatic);

    // String-typed setter that parses the value for you. Used by the
    // console `set` command and the toml loader.
    enum class SetResult : std::uint8_t {
        Ok = 0,
        NotFound,
        TypeMismatch,
        OutOfRange,
        ParseError,
        ReadOnly,
    };
    [[nodiscard]] SetResult set_from_string(std::string_view name,
                                            std::string_view value,
                                            std::uint32_t origin = kOriginConsole);

    // Reset a single CVar to its default. Publishes ConfigCVarChanged.
    void reset_to_default(CVarId, std::uint32_t origin = kOriginProgrammatic);
    // Reset everything under a domain prefix ("audio.", "ui.", ...).
    void reset_domain(std::string_view prefix, std::uint32_t origin = kOriginProgrammatic);

    // Inspection.
    [[nodiscard]] const CVarEntry* entry(CVarId) const noexcept;
    [[nodiscard]] const CVarEntry* entry_by_name(std::string_view) const noexcept;
    [[nodiscard]] std::size_t      count() const noexcept { return entries_.size(); }
    // Iterate all names (sorted by registration order). Used by `help` + `cvars.dump`.
    [[nodiscard]] std::span<const CVarEntry> entries() const noexcept {
        return {entries_.data(), entries_.size()};
    }
    [[nodiscard]] std::uint64_t version() const noexcept { return version_; }

    // True if any CVar with kCVarCheat is not equal to its default value.
    [[nodiscard]] bool cheats_tripped() const noexcept;

private:
    CVarId insert_(const std::string& name, CVarType t,
                   std::uint32_t flags, std::string_view desc);
    void publish_change_(CVarId id, std::uint32_t origin) noexcept;
    [[nodiscard]] static bool parse_bool_(std::string_view, bool& out) noexcept;

    std::vector<CVarEntry>       entries_{};
    std::uint64_t                version_{0};
    events::EventBus<events::ConfigCVarChanged>* bus_{nullptr};
    std::mutex                   mutex_{};
};

} // namespace gw::config
