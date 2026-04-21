// engine/ui/locale_bridge.cpp — ADR-0028 Wave 11D.

#include "engine/ui/locale_bridge.hpp"
#include "engine/core/events/events_core.hpp"  // fnv1a32

#include <algorithm>

namespace gw::ui {

StringId LocaleBridge::make_id(std::string_view key) noexcept {
    return StringId{events::fnv1a32(key)};
}

bool LocaleBridge::set_locale(std::string_view bcp47_tag) noexcept {
    for (const auto& t : tables_) {
        if (t.locale == bcp47_tag) {
            current_.assign(bcp47_tag.data(), bcp47_tag.size());
            return true;
        }
    }
    current_ = "en-US";
    return false;
}

LocaleBridge::Table& LocaleBridge::table_for_(std::string_view locale) {
    for (auto& t : tables_) {
        if (t.locale == locale) return t;
    }
    tables_.push_back(Table{std::string{locale}, {}});
    return tables_.back();
}

void LocaleBridge::register_string(std::string_view locale,
                                   std::string_view key,
                                   std::string_view utf8_value) {
    auto& t = table_for_(locale);
    const std::uint32_t h = events::fnv1a32(key);
    auto it = std::lower_bound(t.rows.begin(), t.rows.end(), h,
                               [](const Row& r, std::uint32_t v) { return r.key_hash < v; });
    if (it != t.rows.end() && it->key_hash == h) {
        it->utf8_value.assign(utf8_value.data(), utf8_value.size());
        return;
    }
    t.rows.insert(it, Row{h, std::string{utf8_value}});
}

const std::string* LocaleBridge::find_(const Table& t, std::uint32_t key_hash) const noexcept {
    auto it = std::lower_bound(t.rows.begin(), t.rows.end(), key_hash,
                               [](const Row& r, std::uint32_t v) { return r.key_hash < v; });
    if (it == t.rows.end() || it->key_hash != key_hash) return nullptr;
    return &it->utf8_value;
}

std::string LocaleBridge::resolve(StringId id, std::string_view key_fallback) const {
    for (const auto& t : tables_) {
        if (t.locale != current_) continue;
        if (auto* s = find_(t, id.value)) return *s;
        break;
    }
    if (current_ != "en-US") {
        for (const auto& t : tables_) {
            if (t.locale != "en-US") continue;
            if (auto* s = find_(t, id.value)) return *s;
            break;
        }
    }
    return std::string{key_fallback};
}

std::vector<LocaleBridge::Entry> LocaleBridge::dump() const {
    std::vector<Entry> out;
    for (const auto& t : tables_) {
        for (const auto& r : t.rows) {
            out.push_back(Entry{t.locale, r.key_hash, r.utf8_value});
        }
    }
    return out;
}

} // namespace gw::ui
