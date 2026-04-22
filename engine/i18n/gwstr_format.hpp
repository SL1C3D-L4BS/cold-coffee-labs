#pragma once
// engine/i18n/gwstr_format.hpp — ADR-0068 §2.

#include <array>
#include <cstddef>
#include <cstdint>

namespace gw::i18n::gwstr {

inline constexpr std::uint32_t kMagic   = 0x52535747u; // 'GWSR' little-endian
inline constexpr std::uint16_t kVersion = 1;

inline constexpr std::uint16_t kFlagBidiHint = 1u << 0;

#pragma pack(push, 1)
struct HeaderPrefix {
    std::uint32_t magic;          // kMagic
    std::uint16_t version;        // kVersion
    std::uint16_t flags;          // kFlag*
    std::uint32_t string_count;
    std::uint32_t blob_size;
    std::uint8_t  locale_tag_len; // BCP-47 byte length
    std::uint8_t  reserved[3];
    // locale_tag[locale_tag_len] follows
};
static_assert(sizeof(HeaderPrefix) == 20, "HeaderPrefix must be 20B");

struct IndexEntry {
    std::uint32_t key_hash;     // fnv1a32
    std::uint32_t utf8_offset;  // into blob
    std::uint32_t utf8_size;
};
static_assert(sizeof(IndexEntry) == 12, "IndexEntry must be 12B");

struct Footer {
    std::uint32_t          reserved;
    std::array<std::uint8_t, 32> blake3_256;
};
static_assert(sizeof(Footer) == 36, "Footer must be 36B");
#pragma pack(pop)

enum class LocaleError : std::uint8_t {
    Ok = 0,
    BadMagic,
    VersionTooNew,
    Truncated,
    HashMismatch,
    InvalidUtf8,
    IndexUnsorted,
};

} // namespace gw::i18n::gwstr
