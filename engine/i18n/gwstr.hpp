#pragma once
// engine/i18n/gwstr.hpp — StringTable reader + writer (ADR-0068).

#include "engine/i18n/gwstr_format.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::i18n {

struct StringRow {
    std::uint32_t key_hash{0};   // fnv1a32 of canonical key
    std::string_view utf8{};     // view into StringTable::blob_
};

class StringTable {
public:
    StringTable() = default;
    StringTable(const StringTable&)            = delete;
    StringTable& operator=(const StringTable&) = delete;
    StringTable(StringTable&&) noexcept        = default;
    StringTable& operator=(StringTable&&) noexcept = default;

    [[nodiscard]] std::string_view locale() const noexcept { return locale_; }
    [[nodiscard]] std::size_t       size()   const noexcept { return rows_.size(); }
    [[nodiscard]] std::uint16_t     flags()  const noexcept { return flags_; }
    [[nodiscard]] std::size_t       blob_size() const noexcept { return blob_.size(); }
    [[nodiscard]] bool has_bidi_hint() const noexcept { return (flags_ & gwstr::kFlagBidiHint) != 0; }

    [[nodiscard]] std::string_view resolve(std::uint32_t key_hash) const noexcept;
    [[nodiscard]] std::string_view resolve_key(std::string_view key) const noexcept;

    // Visiting helper — iterates rows in sorted key-hash order.
    template <class Fn>
    void for_each(Fn&& fn) const {
        for (const auto& r : rows_) {
            fn(StringRow{r.key_hash, row_view_(r)});
        }
    }

    friend gwstr::LocaleError load_gwstr(std::span<const std::byte>, StringTable&,
                                          bool verify_footer) noexcept;
    friend std::vector<std::byte> write_gwstr(std::string_view locale_bcp47,
                                              std::span<const std::pair<std::string, std::string>> kv,
                                              bool include_bidi_hint);

private:
    struct Row {
        std::uint32_t key_hash;
        std::uint32_t blob_off;
        std::uint32_t blob_len;
    };

    [[nodiscard]] std::string_view row_view_(const Row& r) const noexcept {
        if (r.blob_off + r.blob_len > blob_.size()) return {};
        return std::string_view(reinterpret_cast<const char*>(blob_.data() + r.blob_off), r.blob_len);
    }

    std::string       locale_{};
    std::uint16_t     flags_{0};
    std::vector<Row>  rows_{};
    std::vector<std::byte> blob_{};
};

// Parse a `.gwstr` file into a StringTable. The table holds an internal
// copy of the blob bytes so the input `span` does not need to live on.
[[nodiscard]] gwstr::LocaleError load_gwstr(std::span<const std::byte> file,
                                             StringTable& out,
                                             bool verify_footer = true) noexcept;

// Build a deterministic .gwstr from key/value pairs.  Input is sorted,
// NFC-normalised (caller's responsibility — we don't own ICU here), then
// hashed + packed.
[[nodiscard]] std::vector<std::byte>
write_gwstr(std::string_view locale_bcp47,
            std::span<const std::pair<std::string, std::string>> kv,
            bool include_bidi_hint = true);

// Key-hash function — fnv1a32, matching ADR-0028 `LocaleBridge` exactly.
[[nodiscard]] std::uint32_t key_hash(std::string_view key) noexcept;

} // namespace gw::i18n
