#pragma once
// engine/i18n/i18n_config.hpp — ADR-0069.

#include <cstdint>
#include <string>
#include <vector>

namespace gw::i18n {

struct I18nConfig {
    std::string              default_locale{"en-US"};
    std::string              fallback_locale{"en-US"};
    std::string              tables_dir{};          // expanded on init
    std::vector<std::string> preloaded_locales{};   // e.g. {"en-US","fr-FR","de-DE","ja-JP","zh-CN"}
    bool                     enable_bidi{true};
    bool                     enable_harfbuzz{false};
    bool                     verify_table_hashes{true};
    std::int32_t             cache_capacity{1024};
};

} // namespace gw::i18n
