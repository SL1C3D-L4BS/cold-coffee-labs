// engine/i18n/i18n_world.cpp — ADR-0069.

#include "engine/i18n/i18n_world.hpp"

#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/events/event_bus.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace gw::i18n {

namespace {

std::string expand_user_path(std::string s) {
    constexpr std::string_view kTok = "$user";
    const auto pos = s.find(kTok);
    if (pos == std::string::npos) return s;
    std::string base;
#ifdef _WIN32
    if (const char* p = std::getenv("LOCALAPPDATA")) base = p;
    else base = ".";
#else
    if (const char* p = std::getenv("XDG_DATA_HOME")) base = p;
    else if (const char* h = std::getenv("HOME")) base = std::string(h) + "/.local/share";
    else base = ".";
#endif
    base += "/CCL/Greywater";
    s.replace(pos, kTok.size(), base);
    return s;
}

std::vector<std::byte> read_all_bytes(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::vector<std::byte> buf;
    f.seekg(0, std::ios::end);
    const auto n = static_cast<std::streamsize>(f.tellg());
    f.seekg(0, std::ios::beg);
    if (n <= 0) return {};
    buf.resize(static_cast<std::size_t>(n));
    f.read(reinterpret_cast<char*>(buf.data()), n);
    return buf;
}

bool is_valid_bcp47_like(std::string_view s) noexcept {
    if (s.empty() || s.size() > 32) return false;
    for (char c : s) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
    }
    return true;
}

struct TableRecord {
    std::string      bcp47{};
    std::string      source_path{};
    StringTable      table{};
    LocaleStatus     status{};
};

} // namespace

struct I18nWorld::Impl {
    I18nConfig                    cfg{};
    gw::config::CVarRegistry*     cvars{nullptr};
    gw::events::CrossSubsystemBus* bus{nullptr};
    gw::jobs::Scheduler*          sched{nullptr};

    std::string                   active{"en-US"};
    std::string                   fallback{"en-US"};

    std::vector<TableRecord>      tables{};

    bool                          inited{false};
    std::uint64_t                 steps{0};

    TableRecord* find_(std::string_view tag) noexcept {
        for (auto& t : tables) if (t.bcp47 == tag) return &t;
        return nullptr;
    }
    const TableRecord* find_(std::string_view tag) const noexcept {
        for (const auto& t : tables) if (t.bcp47 == tag) return &t;
        return nullptr;
    }
};

I18nWorld::I18nWorld() : impl_(std::make_unique<Impl>()) {}
I18nWorld::~I18nWorld() { shutdown(); }

bool I18nWorld::initialize(I18nConfig                   cfg,
                            gw::config::CVarRegistry*    cvars,
                            gw::events::CrossSubsystemBus* bus,
                            gw::jobs::Scheduler*          scheduler) {
    shutdown();
    impl_->cfg     = std::move(cfg);
    impl_->cvars   = cvars;
    impl_->bus     = bus;
    impl_->sched   = scheduler;

    if (impl_->cfg.tables_dir.empty()) {
        impl_->cfg.tables_dir = "$user/i18n";
    }
    impl_->cfg.tables_dir = expand_user_path(impl_->cfg.tables_dir);
    std::error_code ec;
    std::filesystem::create_directories(impl_->cfg.tables_dir, ec);

    if (impl_->cvars) {
        impl_->cfg.default_locale      = impl_->cvars->get_string_or("i18n.locale",             impl_->cfg.default_locale);
        impl_->cfg.fallback_locale     = impl_->cvars->get_string_or("i18n.fallback_locale",    impl_->cfg.fallback_locale);
        impl_->cfg.enable_bidi         = impl_->cvars->get_bool_or  ("i18n.bidi.enabled",       impl_->cfg.enable_bidi);
        impl_->cfg.enable_harfbuzz     = impl_->cvars->get_bool_or  ("i18n.harfbuzz.enabled",   impl_->cfg.enable_harfbuzz);
        impl_->cfg.verify_table_hashes = impl_->cvars->get_bool_or  ("i18n.verify_table_hashes",impl_->cfg.verify_table_hashes);
    }

    if (!is_valid_bcp47_like(impl_->cfg.default_locale))  impl_->cfg.default_locale  = "en-US";
    if (!is_valid_bcp47_like(impl_->cfg.fallback_locale)) impl_->cfg.fallback_locale = "en-US";

    impl_->active   = impl_->cfg.default_locale;
    impl_->fallback = impl_->cfg.fallback_locale;

    for (const auto& tag : impl_->cfg.preloaded_locales) {
        if (!is_valid_bcp47_like(tag)) continue;
        auto path = std::filesystem::path(impl_->cfg.tables_dir) / (tag + ".gwstr");
        std::error_code ec2;
        if (!std::filesystem::exists(path, ec2)) continue;
        (void)load_table_from_file(tag, path.string());
    }

    impl_->inited = true;
    return true;
}

void I18nWorld::shutdown() {
    impl_->tables.clear();
    impl_->inited = false;
}

bool I18nWorld::initialized() const noexcept { return impl_->inited; }

void I18nWorld::step(double) {
    if (!impl_->inited) return;
    ++impl_->steps;
}

I18nError I18nWorld::set_locale(std::string_view bcp47) {
    if (!impl_->inited) return I18nError::TableNotLoaded;
    if (!is_valid_bcp47_like(bcp47)) return I18nError::BadLocale;
    const std::string prev = impl_->active;
    impl_->active.assign(bcp47.data(), bcp47.size());
    if (impl_->bus) {
        LocaleChanged e{prev, impl_->active};
        (void)e; // CrossSubsystemBus integration handled in sandbox/engine glue.
    }
    return I18nError::Ok;
}

std::string_view I18nWorld::locale() const noexcept { return impl_->active; }
std::string_view I18nWorld::fallback_locale() const noexcept { return impl_->fallback; }

I18nError I18nWorld::load_table_from_bytes(std::string_view           bcp47,
                                             std::span<const std::byte> data,
                                             std::string_view           source_path) {
    if (!impl_->inited) return I18nError::TableNotLoaded;
    if (!is_valid_bcp47_like(bcp47)) return I18nError::BadLocale;

    TableRecord rec{};
    rec.bcp47.assign(bcp47.data(), bcp47.size());
    if (!source_path.empty()) rec.source_path.assign(source_path.data(), source_path.size());

    const auto err = load_gwstr(data, rec.table, impl_->cfg.verify_table_hashes);
    if (err != gwstr::LocaleError::Ok) {
        switch (err) {
            case gwstr::LocaleError::BadMagic:
            case gwstr::LocaleError::VersionTooNew:
            case gwstr::LocaleError::Truncated:
            case gwstr::LocaleError::IndexUnsorted: return I18nError::ParseError;
            case gwstr::LocaleError::HashMismatch:  return I18nError::HashMismatch;
            case gwstr::LocaleError::InvalidUtf8:   return I18nError::InvalidUtf8;
            default:                                  return I18nError::IoError;
        }
    }
    rec.status.bcp47        = rec.bcp47;
    rec.status.source_path  = rec.source_path;
    rec.status.string_count = static_cast<std::uint32_t>(rec.table.size());
    rec.status.blob_bytes   = static_cast<std::uint32_t>(rec.table.blob_size());
    rec.status.loaded       = true;
    rec.status.has_bidi     = rec.table.has_bidi_hint();

    if (auto* existing = impl_->find_(rec.bcp47)) {
        *existing = std::move(rec);
    } else {
        impl_->tables.push_back(std::move(rec));
    }
    return I18nError::Ok;
}

I18nError I18nWorld::load_table_from_file(std::string_view bcp47, std::string_view path) {
    if (!impl_->inited) return I18nError::TableNotLoaded;
    auto bytes = read_all_bytes(std::filesystem::path(std::string(path)));
    if (bytes.empty()) return I18nError::IoError;
    return load_table_from_bytes(bcp47, bytes, path);
}

void I18nWorld::install_inline_table(std::string_view bcp47,
                                       std::span<const std::pair<std::string, std::string>> kv) {
    if (!is_valid_bcp47_like(bcp47)) return;
    auto bytes = write_gwstr(bcp47, kv, true);
    (void)load_table_from_bytes(bcp47, bytes, std::string_view{"<inline>"});
}

std::string_view I18nWorld::resolve(std::string_view key) const noexcept {
    if (!impl_->inited) return {};
    if (auto* t = impl_->find_(impl_->active)) {
        auto v = t->table.resolve_key(key);
        if (!v.empty()) return v;
    }
    if (impl_->active != impl_->fallback) {
        if (auto* t = impl_->find_(impl_->fallback)) {
            auto v = t->table.resolve_key(key);
            if (!v.empty()) return v;
        }
    }
    return {};
}

std::string_view I18nWorld::resolve_in(std::string_view bcp47,
                                          std::string_view key) const noexcept {
    if (!impl_->inited) return {};
    if (auto* t = impl_->find_(bcp47)) return t->table.resolve_key(key);
    return {};
}

std::string I18nWorld::format(std::string_view pattern, std::span<const FmtArg> args) const {
    return format_message(impl_->active, pattern, args);
}

std::string I18nWorld::format_key(std::string_view key, std::span<const FmtArg> args) const {
    auto pat = resolve(key);
    if (pat.empty()) pat = key;
    return format_message(impl_->active, pat, args);
}

std::string I18nWorld::number(std::int64_t v) const {
    auto nf = NumberFormat::make(impl_->active);
    return nf->format(v);
}

std::string I18nWorld::number(double v, std::int32_t frac_digits) const {
    auto nf = NumberFormat::make(impl_->active);
    return nf->format(v, frac_digits);
}

std::string I18nWorld::datetime(std::int64_t unix_ms, std::string_view skeleton) const {
    auto df = DateTimeFormat::make(impl_->active, skeleton);
    return df->format(unix_ms);
}

PluralCategory I18nWorld::plural(double q) const noexcept {
    return plural_category(impl_->active, q);
}

std::size_t  I18nWorld::table_count() const noexcept { return impl_->tables.size(); }

LocaleStatus I18nWorld::status(std::string_view bcp47) const {
    if (auto* t = impl_->find_(bcp47)) return t->status;
    LocaleStatus s{};
    s.bcp47.assign(bcp47.data(), bcp47.size());
    return s;
}

std::vector<LocaleStatus> I18nWorld::statuses() const {
    std::vector<LocaleStatus> out;
    out.reserve(impl_->tables.size());
    for (const auto& t : impl_->tables) out.push_back(t.status);
    return out;
}

bool I18nWorld::has_bidi_in_active_locale() const noexcept {
    if (auto* t = impl_->find_(impl_->active)) return t->status.has_bidi;
    return false;
}

} // namespace gw::i18n
