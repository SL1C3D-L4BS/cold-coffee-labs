#pragma once
// engine/i18n/i18n_cvars.hpp — ADR-0069 §5 (8 i18n.* CVars).

#include "engine/core/config/cvar_registry.hpp"

namespace gw::i18n {

struct I18nCVars {
    config::CVarRef<std::string>  locale{};
    config::CVarRef<std::string>  fallback_locale{};
    config::CVarRef<bool>         bidi_enabled{};
    config::CVarRef<bool>         harfbuzz_enabled{};
    config::CVarRef<bool>         verify_table_hashes{};
    config::CVarRef<std::int32_t> cache_capacity{};
    config::CVarRef<std::string>  tables_dir{};
    config::CVarRef<bool>         dev_hot_reload{};
};

[[nodiscard]] I18nCVars register_i18n_cvars(config::CVarRegistry& r);

} // namespace gw::i18n
