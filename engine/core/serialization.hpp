#pragma once
// engine/core/serialization.hpp
// Low-level binary I/O primitives used by ECS serialization (ADR-0006 §2.9).
//
// Design rules:
//   * Little-endian, natural alignment for scalars (memcpy, not bit-packing).
//   * Reader returns an error sentinel instead of throwing; callers promote.
//   * No allocations beyond the caller-supplied buffer / vector.
//   * Strings are u32 length + raw UTF-8 bytes (no null terminator).
//
// Higher-level ECS framing (headers, archetype tables, chunk payloads) lives
// in engine/ecs/serialize.{hpp,cpp}.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace gw::core {

// ---------------------------------------------------------------------------
// BinaryWriter — appends to a caller-owned byte vector.
// ---------------------------------------------------------------------------
class BinaryWriter {
public:
    explicit BinaryWriter(std::vector<std::uint8_t>& out) noexcept : out_(out) {}

    BinaryWriter(const BinaryWriter&)            = delete;
    BinaryWriter& operator=(const BinaryWriter&) = delete;

    // Raw bytes.
    void write_bytes(const void* data, std::size_t n) {
        const auto* b = static_cast<const std::uint8_t*>(data);
        out_.insert(out_.end(), b, b + n);
    }
    void write_bytes(std::span<const std::uint8_t> data) {
        out_.insert(out_.end(), data.begin(), data.end());
    }

    // Trivially-copyable scalar — memcpy'd little-endian. T must be arithmetic,
    // enum, or a trivially-copyable POD with known layout.
    template <typename T>
    void write(const T& v) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "BinaryWriter::write<T> requires trivially-copyable T");
        const auto n = out_.size();
        out_.resize(n + sizeof(T));
        std::memcpy(out_.data() + n, &v, sizeof(T));
    }

    // String: u32 length + bytes. Never null-terminated on the wire.
    void write_string(std::string_view s) {
        write<std::uint32_t>(static_cast<std::uint32_t>(s.size()));
        write_bytes(s.data(), s.size());
    }

    [[nodiscard]] std::size_t size() const noexcept { return out_.size(); }

private:
    std::vector<std::uint8_t>& out_;
};

// ---------------------------------------------------------------------------
// BinaryReader — read cursor over a borrowed byte span.
// On error (short read, bad length) the cursor is left at the failure point
// and `ok()` returns false. Subsequent reads fail-closed.
// ---------------------------------------------------------------------------
class BinaryReader {
public:
    explicit BinaryReader(std::span<const std::uint8_t> src) noexcept
        : data_(src.data()), size_(src.size()) {}

    [[nodiscard]] bool        ok()        const noexcept { return ok_; }
    [[nodiscard]] std::size_t position()  const noexcept { return pos_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return size_ - pos_; }

    bool read_bytes(void* dst, std::size_t n) {
        if (!ok_ || pos_ + n > size_) { ok_ = false; return false; }
        std::memcpy(dst, data_ + pos_, n);
        pos_ += n;
        return true;
    }

    template <typename T>
    bool read(T& out) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "BinaryReader::read<T> requires trivially-copyable T");
        return read_bytes(&out, sizeof(T));
    }

    template <typename T>
    [[nodiscard]] T read_or(T fallback) {
        T v{};
        return read<T>(v) ? v : fallback;
    }

    bool read_string(std::string& out) {
        std::uint32_t n = 0;
        if (!read<std::uint32_t>(n)) return false;
        if (n > remaining()) { ok_ = false; return false; }
        out.assign(reinterpret_cast<const char*>(data_ + pos_), n);
        pos_ += n;
        return true;
    }

    // Advance without copying; returns a view into the source span.
    [[nodiscard]] std::span<const std::uint8_t> take(std::size_t n) {
        if (!ok_ || pos_ + n > size_) { ok_ = false; return {}; }
        std::span<const std::uint8_t> out{data_ + pos_, n};
        pos_ += n;
        return out;
    }

    // Skip N bytes without materializing them.
    bool skip(std::size_t n) {
        if (!ok_ || pos_ + n > size_) { ok_ = false; return false; }
        pos_ += n;
        return true;
    }

private:
    const std::uint8_t* data_;
    std::size_t         size_;
    std::size_t         pos_ = 0;
    bool                ok_  = true;
};

// ---------------------------------------------------------------------------
// CRC-64 / ISO 3309 (polynomial 0x000000000000001B reflected, init 0xFFFF..FF,
// output xor 0xFFFF..FF). Used by ADR-0006 §2.3 to guard headered payloads.
// ---------------------------------------------------------------------------
[[nodiscard]] std::uint64_t crc64_iso(std::span<const std::uint8_t> data) noexcept;

} // namespace gw::core
