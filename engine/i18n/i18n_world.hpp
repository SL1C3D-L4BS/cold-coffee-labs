#pragma once
// engine/i18n/i18n_world.hpp — ADR-0069 PIMPL façade.

#include "engine/i18n/events_i18n.hpp"
#include "engine/i18n/gwstr.hpp"
#include "engine/i18n/i18n_config.hpp"
#include "engine/i18n/i18n_types.hpp"
#include "engine/i18n/message_format.hpp"
#include "engine/i18n/number_format.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::config { class CVarRegistry; }
namespace gw::events { class CrossSubsystemBus; }
namespace gw::jobs   { class Scheduler; }

namespace gw::i18n {

class I18nWorld {
public:
    I18nWorld();
    ~I18nWorld();
    I18nWorld(const I18nWorld&)            = delete;
    I18nWorld& operator=(const I18nWorld&) = delete;

    [[nodiscard]] bool initialize(I18nConfig                 cfg,
                                   gw::config::CVarRegistry*  cvars,
                                   gw::events::CrossSubsystemBus* bus = nullptr,
                                   gw::jobs::Scheduler*       scheduler = nullptr);
    void               shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    void step(double dt_seconds);

    // Locale management.
    [[nodiscard]] I18nError set_locale(std::string_view bcp47);
    [[nodiscard]] std::string_view locale() const noexcept;
    [[nodiscard]] std::string_view fallback_locale() const noexcept;

    // Table load / register.
    [[nodiscard]] I18nError load_table_from_file(std::string_view bcp47,
                                                   std::string_view path);
    [[nodiscard]] I18nError load_table_from_bytes(std::string_view bcp47,
                                                    std::span<const std::byte> data,
                                                    std::string_view source_path = {});
    // Ingest a list of {key, utf-8 value} pairs directly (used in tests / dev).
    void install_inline_table(std::string_view bcp47,
                                std::span<const std::pair<std::string, std::string>> kv);

    // Resolution.
    [[nodiscard]] std::string_view resolve(std::string_view key) const noexcept;
    [[nodiscard]] std::string_view resolve_in(std::string_view bcp47,
                                                 std::string_view key) const noexcept;
    [[nodiscard]] std::string      format(std::string_view pattern,
                                           std::span<const FmtArg> args) const;
    [[nodiscard]] std::string      format_key(std::string_view key,
                                                std::span<const FmtArg> args) const;

    // Number / date facets.
    [[nodiscard]] std::string number(std::int64_t v) const;
    [[nodiscard]] std::string number(double v, std::int32_t frac_digits = 2) const;
    [[nodiscard]] std::string datetime(std::int64_t unix_ms, std::string_view skeleton = "yMdHms") const;
    [[nodiscard]] PluralCategory plural(double q) const noexcept;

    // Status / introspection.
    [[nodiscard]] std::size_t       table_count() const noexcept;
    [[nodiscard]] LocaleStatus      status(std::string_view bcp47) const;
    [[nodiscard]] std::vector<LocaleStatus> statuses() const;

    // BiDi utility (ADR-0070): returns true when the active locale’s table
    // has the bidi-hint flag set.
    [[nodiscard]] bool has_bidi_in_active_locale() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::i18n
