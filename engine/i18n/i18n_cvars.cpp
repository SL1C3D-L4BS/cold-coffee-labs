// engine/i18n/i18n_cvars.cpp

#include "engine/i18n/i18n_cvars.hpp"

namespace gw::i18n {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kD = config::kCVarDevOnly;
} // namespace

I18nCVars register_i18n_cvars(config::CVarRegistry& r) {
    I18nCVars c{};
    c.locale = r.register_string({
        "i18n.locale", std::string{"en-US"}, kP, {}, {}, false,
        "Active display locale (BCP-47).",
    });
    c.fallback_locale = r.register_string({
        "i18n.fallback_locale", std::string{"en-US"}, kP, {}, {}, false,
        "Fallback locale when a key is missing from the active locale.",
    });
    c.bidi_enabled = r.register_bool({
        "i18n.bidi.enabled", true, kP, {}, {}, false,
        "Enable bidirectional text segmentation.",
    });
    c.harfbuzz_enabled = r.register_bool({
        "i18n.harfbuzz.enabled", false, kP, {}, {}, false,
        "Enable HarfBuzz-based complex script shaping (requires GW_ENABLE_HARFBUZZ).",
    });
    c.verify_table_hashes = r.register_bool({
        "i18n.verify_table_hashes", true, kP, {}, {}, false,
        "Verify content hashes when loading .gwstr tables.",
    });
    c.cache_capacity = r.register_i32({
        "i18n.cache.capacity", 1024, kP, 0, 65536, true,
        "Max cached formatted messages (LRU).",
    });
    c.tables_dir = r.register_string({
        "i18n.tables_dir", std::string{"$user/i18n"}, kP, {}, {}, false,
        "Directory used to resolve <locale>.gwstr at boot.",
    });
    c.dev_hot_reload = r.register_bool({
        "i18n.dev_hot_reload", true, kD, {}, {}, false,
        "Dev builds: re-scan tables_dir on i18n.reload.",
    });
    return c;
}

} // namespace gw::i18n
