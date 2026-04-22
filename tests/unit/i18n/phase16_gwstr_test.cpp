// Phase 16 Wave 16C — .gwstr binary format (ADR-0068).

#include <doctest/doctest.h>
#include <ostream>

#include "engine/i18n/gwstr.hpp"
#include "engine/i18n/gwstr_format.hpp"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

using namespace gw::i18n;

namespace {

std::vector<std::pair<std::string, std::string>> sample_kv() {
    return {
        {"greeting",     "Hello"},
        {"farewell",     "Goodbye"},
        {"confirm",      "OK"},
        {"cancel",       "Cancel"},
    };
}

} // namespace

TEST_CASE("phase16 — key_hash is deterministic & stable") {
    CHECK(key_hash("greeting") == key_hash("greeting"));
    CHECK(key_hash("a") != key_hash("b"));
}

TEST_CASE("phase16 — write_gwstr produces a magic header") {
    const auto bytes = write_gwstr("en-US", sample_kv(), true);
    REQUIRE(bytes.size() >= sizeof(gwstr::HeaderPrefix));
    gwstr::HeaderPrefix hp{};
    std::memcpy(&hp, bytes.data(), sizeof(hp));
    CHECK(hp.magic == gwstr::kMagic);
    CHECK(hp.version == gwstr::kVersion);
    CHECK(hp.string_count == 4u);
}

TEST_CASE("phase16 — load_gwstr round-trips and resolves keys") {
    const auto bytes = write_gwstr("en-US", sample_kv(), true);
    StringTable tab;
    CHECK(load_gwstr(bytes, tab) == gwstr::LocaleError::Ok);
    CHECK(tab.locale() == std::string_view{"en-US"});
    CHECK(tab.size() == 4u);
    CHECK(tab.resolve_key("greeting") == std::string_view{"Hello"});
    CHECK(tab.resolve_key("confirm") == std::string_view{"OK"});
}

TEST_CASE("phase16 — missing keys resolve to empty view") {
    const auto bytes = write_gwstr("en-US", sample_kv(), true);
    StringTable tab;
    REQUIRE(load_gwstr(bytes, tab) == gwstr::LocaleError::Ok);
    CHECK(tab.resolve_key("nope").empty());
}

TEST_CASE("phase16 — header has bidi flag when Hebrew present") {
    const std::vector<std::pair<std::string, std::string>> kv{
        {"shalom", "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d"}, // שלום
    };
    const auto bytes = write_gwstr("he-IL", kv, true);
    gwstr::HeaderPrefix hp{};
    std::memcpy(&hp, bytes.data(), sizeof(hp));
    CHECK((hp.flags & gwstr::kFlagBidiHint) != 0u);
    StringTable tab;
    REQUIRE(load_gwstr(bytes, tab) == gwstr::LocaleError::Ok);
    CHECK(tab.has_bidi_hint());
}

TEST_CASE("phase16 — bidi hint suppressed when no RTL content") {
    const auto bytes = write_gwstr("en-US", sample_kv(), true);
    StringTable tab;
    REQUIRE(load_gwstr(bytes, tab) == gwstr::LocaleError::Ok);
    CHECK_FALSE(tab.has_bidi_hint());
}

TEST_CASE("phase16 — load_gwstr detects bad magic") {
    auto bytes = write_gwstr("en-US", sample_kv(), true);
    std::memset(bytes.data(), 0, 4);
    StringTable tab;
    CHECK(load_gwstr(bytes, tab) == gwstr::LocaleError::BadMagic);
}

TEST_CASE("phase16 — load_gwstr detects truncation") {
    const auto bytes = write_gwstr("en-US", sample_kv(), true);
    std::vector<std::byte> trunc(bytes.begin(), bytes.begin() + 8);
    StringTable tab;
    CHECK(load_gwstr(trunc, tab) == gwstr::LocaleError::Truncated);
}

TEST_CASE("phase16 — load_gwstr detects hash mismatch") {
    auto bytes = write_gwstr("en-US", sample_kv(), true);
    // Flip a byte inside the body.
    bytes[sizeof(gwstr::HeaderPrefix) + 2] = std::byte{0xFF};
    StringTable tab;
    CHECK(load_gwstr(bytes, tab) == gwstr::LocaleError::HashMismatch);
}

TEST_CASE("phase16 — load_gwstr can skip footer verification") {
    auto bytes = write_gwstr("en-US", sample_kv(), true);
    bytes[sizeof(gwstr::HeaderPrefix) + 2] = std::byte{0xFF};
    StringTable tab;
    CHECK(load_gwstr(bytes, tab, /*verify_footer=*/false) == gwstr::LocaleError::Ok);
}

TEST_CASE("phase16 — write_gwstr is deterministic across repeated calls") {
    const auto a = write_gwstr("en-US", sample_kv(), true);
    const auto b = write_gwstr("en-US", sample_kv(), true);
    REQUIRE(a.size() == b.size());
    CHECK(std::memcmp(a.data(), b.data(), a.size()) == 0);
}

TEST_CASE("phase16 — StringTable for_each visits all entries in sorted order") {
    const auto bytes = write_gwstr("en-US", sample_kv(), true);
    StringTable tab;
    REQUIRE(load_gwstr(bytes, tab) == gwstr::LocaleError::Ok);
    std::uint32_t prev = 0;
    std::size_t  count = 0;
    tab.for_each([&](StringRow r) {
        CHECK(r.key_hash >= prev);
        prev = r.key_hash;
        ++count;
    });
    CHECK(count == 4u);
}

TEST_CASE("phase16 — LocaleError enum covers all failure shapes") {
    using E = gwstr::LocaleError;
    CHECK(static_cast<int>(E::Ok)            == 0);
    CHECK(static_cast<int>(E::BadMagic)      != 0);
    CHECK(static_cast<int>(E::VersionTooNew) != 0);
    CHECK(static_cast<int>(E::Truncated)     != 0);
    CHECK(static_cast<int>(E::HashMismatch)  != 0);
    CHECK(static_cast<int>(E::InvalidUtf8)   != 0);
    CHECK(static_cast<int>(E::IndexUnsorted) != 0);
}
