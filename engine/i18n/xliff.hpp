#pragma once
// engine/i18n/xliff.hpp — ADR-0069 §4 (XLIFF 2.1 round-trip).
//
// Minimal reader/writer: we emit fully canonical XLIFF 2.1 with deterministic
// attribute order so that translator round-trips produce stable diffs.  The
// null shim supports `<unit id="…"><segment><source>…</source>
// <target>…</target></segment></unit>` form.

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gw::i18n::xliff {

struct Unit {
    std::string id{};
    std::string source_utf8{};
    std::string target_utf8{};
    std::string notes{};
};

enum class XliffError : std::uint8_t {
    Ok = 0,
    BadRoot,
    MissingFile,
    UnitParseError,
    EncodingError,
};

// Parse an XLIFF 2.1 document.  Appends to `out` (does not clear first).
[[nodiscard]] XliffError parse(std::string_view xml, std::vector<Unit>& out);

// Serialise units to XLIFF 2.1.  `src_lang` and `tgt_lang` are BCP-47 tags.
[[nodiscard]] std::string write(std::string_view src_lang,
                                 std::string_view tgt_lang,
                                 const std::vector<Unit>& units);

// Convenience: convert an XLIFF corpus to the (key,value) pairs consumed by
// write_gwstr().  Only targets that are non-empty are emitted.
[[nodiscard]] std::vector<std::pair<std::string, std::string>>
    to_kv(const std::vector<Unit>& units);

} // namespace gw::i18n::xliff
