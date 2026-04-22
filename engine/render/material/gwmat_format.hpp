#pragma once
// engine/render/material/gwmat_format.hpp — ADR-0076 .gwmat v1 layout.

#include <cstdint>

namespace gw::render::material {

// Binary layout constants (frozen). ADR-0076 §2.
constexpr std::uint32_t kGwMatMagic       = 0x474D5754u;   // 'GWMT' little-endian
constexpr std::uint32_t kGwMatFooterMagic = 0x4557474F;    // 'EOGW'... (illustrative, see encoder)
constexpr std::uint16_t kGwMatMajor       = 1;
constexpr std::uint16_t kGwMatMinor       = 0;
constexpr std::uint32_t kGwMatHeaderSize  = 32;
constexpr std::uint32_t kGwMatMaxParamBlock = 256;
constexpr std::uint16_t kGwMatMaxSlots    = 16;
constexpr std::uint16_t kGwMatMaxNameLen  = 128;

// Flag bits (header[+8]).
enum GwMatFlags : std::uint32_t {
    kGwMatFlagNone       = 0,
    kGwMatFlagCompressed = 1u << 0,
};

// Header laid out on disk (little-endian).
#pragma pack(push, 1)
struct GwMatHeader {
    std::uint32_t magic;               // +0
    std::uint16_t major;               // +4
    std::uint16_t minor;               // +6
    std::uint32_t flags;               // +8
    std::uint32_t payload_size;        // +12
    std::uint64_t content_hash;        // +16
    std::uint32_t param_block_size;    // +24
    std::uint16_t slot_count;          // +28
    std::uint16_t template_name_len;   // +30
};
static_assert(sizeof(GwMatHeader) == 32, "GwMatHeader must be 32 B (frozen)");
#pragma pack(pop)

enum class GwMatError : std::uint8_t {
    Ok = 0,
    MagicMismatch,
    VersionUnsupported,
    HashMismatch,
    Truncated,
    TemplateUnknown,
    ParamLayoutMismatch,
    IoError,
};

[[nodiscard]] const char* to_cstring(GwMatError) noexcept;

} // namespace gw::render::material
