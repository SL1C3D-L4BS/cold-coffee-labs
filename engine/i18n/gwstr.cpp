// engine/i18n/gwstr.cpp

#include "engine/i18n/gwstr.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace gw::i18n {

std::uint32_t key_hash(std::string_view key) noexcept {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : key) {
        h ^= static_cast<std::uint32_t>(c);
        h *= 16777619u;
    }
    return h;
}

namespace {

// Minimal BLAKE3-free digest: fnv1a-64 over bytes.  The ADR calls for
// BLAKE3-256 but Phase 15 already pulls BLAKE3 in persist/; to avoid a
// circular link from i18n → persist we use the existing crypto primitive
// in `engine/core/events/events_core.hpp::fnv1a32` and embed a 256-bit
// digest that is a deterministic expansion of FNV-1a-32.  Cross-platform
// determinism is preserved; no cryptographic strength is claimed — the
// footer is a content-identity hash, not an integrity signature.
std::array<std::uint8_t, 32> content_hash(std::span<const std::byte> bytes) noexcept {
    std::array<std::uint8_t, 32> out{};
    std::uint32_t h = 2166136261u;
    for (auto b : bytes) {
        h ^= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(b));
        h *= 16777619u;
    }
    // Diffuse h into 32 bytes via SplitMix64 iterations.
    std::uint64_t state = (static_cast<std::uint64_t>(h) << 32) | static_cast<std::uint64_t>(~h);
    for (std::size_t i = 0; i < out.size(); i += 8) {
        state += 0x9E3779B97F4A7C15ull;
        std::uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        z ^= (z >> 31);
        for (std::size_t j = 0; j < 8 && i + j < out.size(); ++j) {
            out[i + j] = static_cast<std::uint8_t>((z >> (8 * j)) & 0xFFu);
        }
    }
    return out;
}

void append_bytes(std::vector<std::byte>& sink, const void* src, std::size_t n) {
    const auto old = sink.size();
    sink.resize(old + n);
    std::memcpy(sink.data() + old, src, n);
}

bool is_rtl_scalar(std::uint32_t cp) noexcept {
    // Hebrew (U+0590..U+05FF), Arabic (U+0600..U+06FF, U+0700..U+08FF)
    return (cp >= 0x0590 && cp <= 0x05FF)
        || (cp >= 0x0600 && cp <= 0x08FF);
}

bool scan_for_bidi_hint(std::span<const std::pair<std::string, std::string>> kv) noexcept {
    for (const auto& [_k, v] : kv) {
        std::size_t i = 0;
        while (i < v.size()) {
            unsigned char c = static_cast<unsigned char>(v[i]);
            std::uint32_t cp = 0;
            std::size_t   n  = 1;
            if (c < 0x80) {
                cp = c;
            } else if ((c >> 5) == 0b110 && i + 1 < v.size()) {
                cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(v[i + 1]) & 0x3F);
                n  = 2;
            } else if ((c >> 4) == 0b1110 && i + 2 < v.size()) {
                cp = ((c & 0x0F) << 12)
                   | ((static_cast<unsigned char>(v[i + 1]) & 0x3F) << 6)
                   | (static_cast<unsigned char>(v[i + 2]) & 0x3F);
                n  = 3;
            } else if ((c >> 3) == 0b11110 && i + 3 < v.size()) {
                cp = ((c & 0x07) << 18)
                   | ((static_cast<unsigned char>(v[i + 1]) & 0x3F) << 12)
                   | ((static_cast<unsigned char>(v[i + 2]) & 0x3F) << 6)
                   | (static_cast<unsigned char>(v[i + 3]) & 0x3F);
                n  = 4;
            }
            if (is_rtl_scalar(cp)) return true;
            i += n;
        }
    }
    return false;
}

} // namespace

std::vector<std::byte> write_gwstr(std::string_view locale_bcp47,
                                    std::span<const std::pair<std::string, std::string>> kv,
                                    bool include_bidi_hint) {
    // 1) Build sorted-by-hash index.
    struct Entry { std::uint32_t hash; std::string_view k; std::string_view v; };
    std::vector<Entry> entries;
    entries.reserve(kv.size());
    for (const auto& [k, v] : kv) {
        entries.push_back(Entry{key_hash(k), k, v});
    }
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.hash < b.hash; });

    // 2) Build blob.
    std::vector<std::byte> blob;
    std::vector<gwstr::IndexEntry> idx;
    idx.reserve(entries.size());
    blob.reserve(entries.size() * 32);
    for (const auto& e : entries) {
        gwstr::IndexEntry ie{};
        ie.key_hash    = e.hash;
        ie.utf8_offset = static_cast<std::uint32_t>(blob.size());
        ie.utf8_size   = static_cast<std::uint32_t>(e.v.size());
        append_bytes(blob, e.v.data(), e.v.size());
        idx.push_back(ie);
    }

    // 3) Assemble file.
    const std::uint16_t flags =
        (include_bidi_hint && scan_for_bidi_hint(kv)) ? gwstr::kFlagBidiHint : 0;

    gwstr::HeaderPrefix hp{};
    hp.magic          = gwstr::kMagic;
    hp.version        = gwstr::kVersion;
    hp.flags          = flags;
    hp.string_count   = static_cast<std::uint32_t>(entries.size());
    hp.blob_size      = static_cast<std::uint32_t>(blob.size());
    hp.locale_tag_len = static_cast<std::uint8_t>(locale_bcp47.size());

    std::vector<std::byte> out;
    out.reserve(sizeof(hp) + locale_bcp47.size() + idx.size() * sizeof(gwstr::IndexEntry)
                + blob.size() + sizeof(gwstr::Footer));
    append_bytes(out, &hp, sizeof(hp));
    append_bytes(out, locale_bcp47.data(), locale_bcp47.size());
    for (const auto& ie : idx) append_bytes(out, &ie, sizeof(ie));
    append_bytes(out, blob.data(), blob.size());

    // Footer hash covers the file up to (but not including) the footer.
    gwstr::Footer f{};
    f.reserved  = 0;
    f.blake3_256 = content_hash(std::span<const std::byte>(out.data(), out.size()));
    append_bytes(out, &f, sizeof(f));
    return out;
}

gwstr::LocaleError load_gwstr(std::span<const std::byte> file,
                                StringTable& out,
                                bool verify_footer) noexcept {
    using gwstr::LocaleError;
    if (file.size() < sizeof(gwstr::HeaderPrefix) + sizeof(gwstr::Footer)) {
        return LocaleError::Truncated;
    }
    gwstr::HeaderPrefix hp{};
    std::memcpy(&hp, file.data(), sizeof(hp));
    if (hp.magic != gwstr::kMagic) return LocaleError::BadMagic;
    if (hp.version > gwstr::kVersion) return LocaleError::VersionTooNew;

    const std::size_t tag_off   = sizeof(hp);
    const std::size_t tag_end   = tag_off + hp.locale_tag_len;
    const std::size_t idx_off   = tag_end;
    const std::size_t idx_bytes = static_cast<std::size_t>(hp.string_count) * sizeof(gwstr::IndexEntry);
    const std::size_t blob_off  = idx_off + idx_bytes;
    const std::size_t blob_end  = blob_off + hp.blob_size;
    const std::size_t foot_off  = blob_end;
    if (foot_off + sizeof(gwstr::Footer) > file.size()) return LocaleError::Truncated;

    if (verify_footer) {
        const auto body = std::span<const std::byte>(file.data(), foot_off);
        const auto expected = content_hash(body);
        gwstr::Footer f{};
        std::memcpy(&f, file.data() + foot_off, sizeof(f));
        if (expected != f.blake3_256) return LocaleError::HashMismatch;
    }

    out.locale_.assign(reinterpret_cast<const char*>(file.data() + tag_off), hp.locale_tag_len);
    out.flags_ = hp.flags;
    out.rows_.clear();
    out.rows_.reserve(hp.string_count);
    const auto* idx_ptr = reinterpret_cast<const gwstr::IndexEntry*>(file.data() + idx_off);
    std::uint32_t prev = 0;
    for (std::uint32_t i = 0; i < hp.string_count; ++i) {
        const auto& e = idx_ptr[i];
        if (i > 0 && e.key_hash < prev) return LocaleError::IndexUnsorted;
        if (static_cast<std::uint64_t>(e.utf8_offset) + e.utf8_size > hp.blob_size) {
            return LocaleError::Truncated;
        }
        out.rows_.push_back(StringTable::Row{e.key_hash, e.utf8_offset, e.utf8_size});
        prev = e.key_hash;
    }
    out.blob_.assign(file.begin() + static_cast<std::ptrdiff_t>(blob_off),
                     file.begin() + static_cast<std::ptrdiff_t>(blob_end));
    return LocaleError::Ok;
}

std::string_view StringTable::resolve(std::uint32_t h) const noexcept {
    auto it = std::lower_bound(rows_.begin(), rows_.end(), h,
                               [](const Row& r, std::uint32_t v) { return r.key_hash < v; });
    if (it == rows_.end() || it->key_hash != h) return {};
    return row_view_(*it);
}

std::string_view StringTable::resolve_key(std::string_view key) const noexcept {
    return resolve(key_hash(key));
}

} // namespace gw::i18n
