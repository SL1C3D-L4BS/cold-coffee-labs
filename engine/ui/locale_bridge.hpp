#pragma once
// engine/ui/locale_bridge.hpp — StringId → UTF-8 placeholder (ADR-0028).
//
// Phase 11 ships a *skeleton* LocaleBridge: strings are registered at boot
// from a simple per-locale TOML table (ui.strings.<locale>.toml). Lookup is
// O(log N) via a sorted vector of {hash, utf8}. Fallback is the English
// table. Phase 16 replaces this with ICU-backed bundle resolution, plural
// rules, and bidi segmentation (docs/16 §LOC).

#include "engine/ui/ui_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gw::ui {

class LocaleBridge {
public:
    LocaleBridge() = default;
    ~LocaleBridge() = default;
    LocaleBridge(const LocaleBridge&) = delete;
    LocaleBridge& operator=(const LocaleBridge&) = delete;

    // Switch the active locale tag (e.g. "en-US", "fr-FR"). Returns true if
    // the tag is registered; otherwise falls back to the English table.
    bool set_locale(std::string_view bcp47_tag) noexcept;
    [[nodiscard]] std::string_view locale() const noexcept { return current_; }

    // Register a key/value pair into the current locale's table. Keys are
    // hashed with fnv1a32; a collision is a programming error (assert).
    void register_string(std::string_view locale,
                         std::string_view key,
                         std::string_view utf8_value);

    // Resolve a StringId against the current locale, falling back to "en-US".
    // Returns the literal key text if no translation is available.
    [[nodiscard]] std::string resolve(StringId id, std::string_view key_fallback = {}) const;

    // Build a StringId from a key at compile-time-friendly callsites.
    [[nodiscard]] static StringId make_id(std::string_view key) noexcept;

    // Dev helper — list every (locale, key_hash) pair currently registered.
    // Used by the `loc.dump` console command.
    struct Entry {
        std::string   locale{};
        std::uint32_t key_hash{0};
        std::string   utf8_value{};
    };
    [[nodiscard]] std::vector<Entry> dump() const;

private:
    struct Row { std::uint32_t key_hash; std::string utf8_value; };
    struct Table {
        std::string      locale{};
        std::vector<Row> rows{};   // sorted by key_hash
    };

    Table& table_for_(std::string_view locale);
    [[nodiscard]] const std::string* find_(const Table& t, std::uint32_t key_hash) const noexcept;

    std::vector<Table> tables_{};
    std::string        current_{"en-US"};
};

} // namespace gw::ui
