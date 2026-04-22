// Phase 16 Wave 16D — I18nWorld facade (ADR-0069).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/core/config/cvar_registry.hpp"
#include "engine/i18n/i18n_cvars.hpp"
#include "engine/i18n/i18n_world.hpp"

#include <string>
#include <utility>
#include <vector>

using namespace gw::i18n;

namespace {

std::vector<std::pair<std::string, std::string>> en_kv() {
    return {{"hello","Hello"},{"goodbye","Goodbye"},{"count","{n, plural, one {1 item} other {# items}}"}};
}
std::vector<std::pair<std::string, std::string>> fr_kv() {
    return {{"hello","Bonjour"},{"goodbye","Au revoir"}};
}

} // namespace

TEST_CASE("phase16 — I18nWorld initializes with CVar registry") {
    gw::config::CVarRegistry reg;
    (void)register_i18n_cvars(reg);
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, &reg));
    CHECK(w.initialized());
    CHECK(w.locale() == std::string_view{"en-US"});
    CHECK(w.fallback_locale() == std::string_view{"en-US"});
}

TEST_CASE("phase16 — I18nWorld inline table resolves keys") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.install_inline_table("en-US", en_kv());
    CHECK(w.resolve("hello") == std::string_view{"Hello"});
    CHECK(w.resolve("goodbye") == std::string_view{"Goodbye"});
}

TEST_CASE("phase16 — I18nWorld falls back to fallback locale when key missing") {
    I18nWorld w;
    I18nConfig cfg{};
    cfg.default_locale  = "fr-FR";
    cfg.fallback_locale = "en-US";
    REQUIRE(w.initialize(cfg, nullptr));
    w.install_inline_table("en-US", en_kv());
    w.install_inline_table("fr-FR", fr_kv());
    CHECK(w.resolve("hello") == std::string_view{"Bonjour"});
    // `count` only exists in en-US.
    CHECK(w.resolve("count") == std::string_view{"{n, plural, one {1 item} other {# items}}"});
}

TEST_CASE("phase16 — I18nWorld set_locale flips active tag") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.install_inline_table("fr-FR", fr_kv());
    CHECK(w.set_locale("fr-FR") == I18nError::Ok);
    CHECK(w.locale() == std::string_view{"fr-FR"});
    CHECK(w.resolve("hello") == std::string_view{"Bonjour"});
}

TEST_CASE("phase16 — I18nWorld set_locale rejects garbage tag") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    CHECK(w.set_locale("!!!") == I18nError::BadLocale);
}

TEST_CASE("phase16 — I18nWorld format_key expands plural patterns") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.install_inline_table("en-US", en_kv());
    std::vector<FmtArg> args{FmtArg::of_i64("n", 5)};
    CHECK(w.format_key("count", args) == std::string{"5 items"});
}

TEST_CASE("phase16 — I18nWorld number() uses active locale grouping") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    CHECK(w.number(1234567LL) == std::string{"1,234,567"});
    (void)w.set_locale("fr-FR");
    CHECK(w.number(1234567LL) == std::string{"1.234.567"});
}

TEST_CASE("phase16 — I18nWorld datetime returns ISO-like string") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    const auto s = w.datetime(1700000000LL * 1000, "yMdHms");
    CHECK(s.size() >= 19u);
}

TEST_CASE("phase16 — I18nWorld status reflects loaded tables") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.install_inline_table("en-US", en_kv());
    const auto s = w.status("en-US");
    CHECK(s.loaded);
    CHECK(s.string_count == 3u);
    CHECK(s.blob_bytes > 0u);
}

TEST_CASE("phase16 — I18nWorld statuses lists every installed table") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.install_inline_table("en-US", en_kv());
    w.install_inline_table("fr-FR", fr_kv());
    const auto s = w.statuses();
    CHECK(s.size() == 2u);
}

TEST_CASE("phase16 — I18nWorld has_bidi_in_active_locale reflects bidi flag") {
    I18nWorld w;
    I18nConfig cfg{};
    cfg.default_locale = "he-IL";
    REQUIRE(w.initialize(cfg, nullptr));
    std::vector<std::pair<std::string,std::string>> kv{
        {"shalom", "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d"}, // שלום
    };
    w.install_inline_table("he-IL", kv);
    CHECK(w.has_bidi_in_active_locale());
}

TEST_CASE("phase16 — I18nCVars registers 8 keys") {
    gw::config::CVarRegistry reg;
    const auto c = register_i18n_cvars(reg);
    CHECK(c.locale.valid());
    CHECK(c.fallback_locale.valid());
    CHECK(c.bidi_enabled.valid());
    CHECK(c.harfbuzz_enabled.valid());
    CHECK(c.verify_table_hashes.valid());
    CHECK(c.cache_capacity.valid());
    CHECK(c.tables_dir.valid());
    CHECK(c.dev_hot_reload.valid());
}

TEST_CASE("phase16 — I18nWorld shutdown is idempotent") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.shutdown();
    w.shutdown();
    CHECK_FALSE(w.initialized());
}

TEST_CASE("phase16 — I18nWorld step() increments without crashing") {
    I18nWorld w;
    REQUIRE(w.initialize(I18nConfig{}, nullptr));
    w.step(0.016);
    w.step(0.016);
}

TEST_CASE("phase16 — I18nError to_string covers every enum") {
    for (int i = 0; i <= static_cast<int>(I18nError::HarfBuzzDisabled); ++i) {
        CHECK(!to_string(static_cast<I18nError>(i)).empty());
    }
}
