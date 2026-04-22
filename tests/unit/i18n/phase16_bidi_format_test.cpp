// Phase 16 — BiDi segmentation + number/date/message formatting (ADR-0069/0070).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/i18n/bidi.hpp"
#include "engine/i18n/message_format.hpp"
#include "engine/i18n/number_format.hpp"
#include "engine/i18n/xliff.hpp"

#include <string>
#include <vector>

using namespace gw::i18n;

TEST_CASE("phase16 — bidi resolve_base_direction: ASCII is LTR") {
    CHECK(resolve_base_direction("Hello",  BaseDirection::Auto) == BaseDirection::LTR);
}

TEST_CASE("phase16 — bidi resolve_base_direction: Hebrew is RTL") {
    CHECK(resolve_base_direction("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d", BaseDirection::Auto)
          == BaseDirection::RTL);
}

TEST_CASE("phase16 — bidi explicit base direction is honored") {
    CHECK(resolve_base_direction("Hello", BaseDirection::RTL) == BaseDirection::RTL);
}

TEST_CASE("phase16 — bidi empty string defaults to LTR") {
    CHECK(resolve_base_direction("", BaseDirection::Auto) == BaseDirection::LTR);
}

TEST_CASE("phase16 — bidi segments a pure LTR string into one run") {
    std::vector<BidiRun> runs;
    CHECK(bidi_segments("Hello World", BaseDirection::LTR, runs) == 1u);
    CHECK(runs[0].direction == gw::ui::TextDirection::LTR);
}

TEST_CASE("phase16 — bidi segments a mixed LTR+RTL string into >1 runs") {
    std::vector<BidiRun> runs;
    const std::string s = "Hi " + std::string("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d");
    const auto n = bidi_segments(s, BaseDirection::Auto, runs);
    CHECK(n >= 2u);
    bool saw_rtl = false;
    for (const auto& r : runs) if (r.direction == gw::ui::TextDirection::RTL) saw_rtl = true;
    CHECK(saw_rtl);
}

TEST_CASE("phase16 — NumberFormat groups en-US thousands with comma") {
    auto nf = NumberFormat::make("en-US");
    CHECK(nf->format(1'234'567LL) == std::string{"1,234,567"});
}

TEST_CASE("phase16 — NumberFormat groups fr-FR thousands with dot") {
    auto nf = NumberFormat::make("fr-FR");
    CHECK(nf->format(1'234'567LL) == std::string{"1.234.567"});
}

TEST_CASE("phase16 — NumberFormat formats negatives correctly") {
    auto nf = NumberFormat::make("en-US");
    CHECK(nf->format(-42LL) == std::string{"-42"});
}

TEST_CASE("phase16 — NumberFormat fractional digits rounded") {
    auto nf = NumberFormat::make("en-US");
    CHECK(nf->format(3.14159, 2) == std::string{"3.14"});
}

TEST_CASE("phase16 — NumberFormat fractional uses comma in fr-FR") {
    auto nf = NumberFormat::make("fr-FR");
    CHECK(nf->format(3.5, 1) == std::string{"3,5"});
}

TEST_CASE("phase16 — DateTimeFormat skeleton `y` returns four-digit year") {
    auto df = DateTimeFormat::make("en-US", "y");
    // 1700000000 * 1000 = 2023-11-14T22:13:20Z.
    const std::string s = df->format(1700000000LL * 1000);
    CHECK(s == std::string{"2023"});
}

TEST_CASE("phase16 — DateTimeFormat `yMd` returns YYYY-MM-DD") {
    auto df = DateTimeFormat::make("en-US", "yMd");
    const std::string s = df->format(1700000000LL * 1000);
    CHECK(s == std::string{"2023-11-14"});
}

TEST_CASE("phase16 — plural_category: English rules") {
    CHECK(plural_category("en-US", 1.0) == PluralCategory::One);
    CHECK(plural_category("en-US", 2.0) == PluralCategory::Other);
    CHECK(plural_category("en-US", 0.0) == PluralCategory::Other);
}

TEST_CASE("phase16 — plural_category: French one covers {0,1}") {
    CHECK(plural_category("fr-FR", 0.0) == PluralCategory::One);
    CHECK(plural_category("fr-FR", 1.0) == PluralCategory::One);
    CHECK(plural_category("fr-FR", 2.0) == PluralCategory::Other);
}

TEST_CASE("phase16 — plural_category: Japanese collapses to Other") {
    CHECK(plural_category("ja-JP", 1.0) == PluralCategory::Other);
    CHECK(plural_category("ja-JP", 2.0) == PluralCategory::Other);
}

TEST_CASE("phase16 — to_lower_locale / to_upper_locale are available") {
    CHECK(to_lower_locale("ABC", "en-US") == std::string{"abc"});
    CHECK(to_upper_locale("abc", "en-US") == std::string{"ABC"});
}

TEST_CASE("phase16 — message_format: simple substitution") {
    std::vector<FmtArg> args{FmtArg::of_str("name", "Kira")};
    CHECK(format_message("en-US", "Hello {name}", args) == std::string{"Hello Kira"});
}

TEST_CASE("phase16 — message_format: escape '{{' / '}}'") {
    CHECK(format_message("en-US", "{{raw}}", {}) == std::string{"{raw}"});
}

TEST_CASE("phase16 — message_format: number arg with grouping") {
    std::vector<FmtArg> args{FmtArg::of_i64("count", 12345)};
    CHECK(format_message("en-US", "{count, number}", args) == std::string{"12,345"});
}

TEST_CASE("phase16 — message_format: number arg with fixed-frac") {
    std::vector<FmtArg> args{FmtArg::of_f64("x", 3.14159)};
    CHECK(format_message("en-US", "{x, number, :.2}", args) == std::string{"3.14"});
}

TEST_CASE("phase16 — message_format: plural arms pick `one`") {
    std::vector<FmtArg> args{FmtArg::of_i64("n", 1)};
    CHECK(format_message("en-US",
            "{n, plural, one {1 item} other {# items}}", args)
          == std::string{"1 item"});
}

TEST_CASE("phase16 — message_format: plural arms pick `other` + # shorthand") {
    std::vector<FmtArg> args{FmtArg::of_i64("n", 5)};
    CHECK(format_message("en-US",
            "{n, plural, one {1 item} other {# items}}", args)
          == std::string{"5 items"});
}

TEST_CASE("phase16 — message_format: unknown name preserved as literal brace") {
    std::vector<FmtArg> args{};
    const auto s = format_message("en-US", "{ghost}", args);
    CHECK(s == std::string{"{ghost}"});
}

TEST_CASE("phase16 — xliff: write/parse round-trip is deterministic") {
    std::vector<xliff::Unit> units;
    units.push_back({"hello",   "Hello",   "Bonjour",  {}});
    units.push_back({"goodbye", "Goodbye", "Au revoir", "polite form"});
    const std::string xml = xliff::write("en-US", "fr-FR", units);
    std::vector<xliff::Unit> parsed;
    CHECK(xliff::parse(xml, parsed) == xliff::XliffError::Ok);
    REQUIRE(parsed.size() == 2u);
    CHECK(parsed[0].id == std::string{"goodbye"}); // alphabetical sort
    CHECK(parsed[1].source_utf8 == std::string{"Hello"});
    CHECK(parsed[1].target_utf8 == std::string{"Bonjour"});
}

TEST_CASE("phase16 — xliff: missing <file> is an error") {
    std::vector<xliff::Unit> parsed;
    CHECK(xliff::parse("<xliff></xliff>", parsed) == xliff::XliffError::MissingFile);
}

TEST_CASE("phase16 — xliff: to_kv emits only non-empty targets") {
    std::vector<xliff::Unit> units;
    units.push_back({"a", "A", "Alfa", {}});
    units.push_back({"b", "B", "",     {}});
    const auto kv = xliff::to_kv(units);
    REQUIRE(kv.size() == 1u);
    CHECK(kv[0].first  == std::string{"a"});
    CHECK(kv[0].second == std::string{"Alfa"});
}
