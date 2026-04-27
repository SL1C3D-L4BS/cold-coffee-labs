// engine/runtime/progs_vm.cpp — ADR-0122 bounded interpreter (no Quake code).
#include "engine/runtime/progs_vm.hpp"

#include <array>
#include <cstring>
#include <vector>

namespace gw::runtime::progs {
namespace {

constexpr std::uint32_t kMagicGwpgLe = 0x47575047u;  // 'GWPG'
constexpr std::uint16_t kVersion    = 1;
constexpr std::size_t   kHeaderBytes = 12;
constexpr std::size_t   kMaxStack    = 256;

[[nodiscard]] std::uint16_t read_u16_le(const std::byte* p) noexcept {
    std::uint16_t v{};
    std::memcpy(&v, p, sizeof(v));
    return v;
}

[[nodiscard]] std::uint32_t read_u32_le(const std::byte* p) noexcept {
    std::uint32_t v{};
    std::memcpy(&v, p, sizeof(v));
    return v;
}

[[nodiscard]] VerifyResult verify_words(std::span<const std::uint16_t> words) noexcept {
    std::size_t pc = 0;
    while (pc < words.size()) {
        const auto op = static_cast<Opcode>(words[pc]);
        switch (op) {
        case Opcode::Halt:
            ++pc;
            break;
        case Opcode::PushImm:
            if (pc + 3 > words.size()) {
                return VerifyResult::TruncatedPush;
            }
            pc += 3;
            break;
        case Opcode::Add:
        case Opcode::Dup:
        case Opcode::Pop:
            ++pc;
            break;
        default:
            return VerifyResult::UnknownOpcode;
        }
    }
    return VerifyResult::Ok;
}

std::vector<std::byte> g_module_bytes{};
bool                   g_module_verified{false};

}  // namespace

VerifyResult verify_bytecode(std::span<const std::byte> blob) noexcept {
    if (blob.size() < kHeaderBytes) {
        return VerifyResult::TooSmall;
    }
    const std::byte* base = blob.data();
    if (read_u32_le(base) != kMagicGwpgLe) {
        return VerifyResult::BadMagic;
    }
    if (read_u16_le(base + 4) != kVersion) {
        return VerifyResult::BadVersion;
    }
    const std::uint32_t word_count = read_u32_le(base + 8);
    if (word_count > 1'000'000u) {
        return VerifyResult::SizeMismatch;
    }
    const std::size_t need = kHeaderBytes + static_cast<std::size_t>(word_count) * 2u;
    if (blob.size() != need) {
        return VerifyResult::SizeMismatch;
    }
    std::vector<std::uint16_t> words(word_count);
    for (std::uint32_t i = 0; i < word_count; ++i) {
        words[i] = read_u16_le(base + kHeaderBytes + static_cast<std::size_t>(i) * 2u);
    }
    return verify_words(words);
}

RunResult run_bytecode(std::span<const std::byte> blob, std::uint32_t max_cycles) noexcept {
    RunResult out{};
    const VerifyResult vr = verify_bytecode(blob);
    if (vr != VerifyResult::Ok) {
        return out;
    }
    const std::byte* base = blob.data();
    const std::uint32_t word_count = read_u32_le(base + 8);
    std::array<std::int32_t, kMaxStack> stack{};
    std::size_t sp = 0;
    std::size_t pc = 0;
    std::uint32_t cycles = 0;

    while (pc < word_count && cycles < max_cycles) {
        const auto op = static_cast<Opcode>(read_u16_le(base + kHeaderBytes + pc * 2u));
        ++cycles;
        switch (op) {
        case Opcode::Halt:
            out.ok           = true;
            out.cycles_used  = cycles;
            out.stack_empty  = (sp == 0);
            if (sp > 0) {
                out.stack_top = stack[sp - 1];
            }
            return out;
        case Opcode::PushImm: {
            if (pc + 3 > word_count) {
                return out;
            }
            const std::uint16_t lo =
                read_u16_le(base + kHeaderBytes + (pc + 1) * 2u);
            const std::uint16_t hi =
                read_u16_le(base + kHeaderBytes + (pc + 2) * 2u);
            std::uint32_t imm_u{};
            imm_u = (static_cast<std::uint32_t>(hi) << 16) | static_cast<std::uint32_t>(lo);
            if (sp >= kMaxStack) {
                return out;
            }
            std::int32_t v{};
            std::memcpy(&v, &imm_u, sizeof(v));
            stack[sp++] = v;
            pc += 3;
            break;
        }
        case Opcode::Add:
            if (sp < 2) {
                return out;
            }
            {
                const std::int32_t b = stack[--sp];
                const std::int32_t a = stack[--sp];
                stack[sp++] = a + b;
            }
            ++pc;
            break;
        case Opcode::Dup:
            if (sp < 1 || sp >= kMaxStack) {
                return out;
            }
            stack[sp] = stack[sp - 1];
            ++sp;
            ++pc;
            break;
        case Opcode::Pop:
            if (sp < 1) {
                return out;
            }
            --sp;
            ++pc;
            break;
        default:
            return out;
        }
    }
    // Fell off end of program (verify should ensure last op is Halt for clean
    // modules); still report stack + cycles for diagnostics.
    out.ok          = false;
    out.cycles_used = cycles;
    out.stack_empty = (sp == 0);
    if (sp > 0) {
        out.stack_top = stack[sp - 1];
    }
    return out;
}

void set_active_bytecode(std::span<const std::byte> blob) noexcept {
    g_module_verified = false;
    if (blob.empty()) {
        g_module_bytes.clear();
        return;
    }
    g_module_bytes.assign(blob.begin(), blob.end());
    if (verify_bytecode(g_module_bytes) != VerifyResult::Ok) {
        g_module_bytes.clear();
        return;
    }
    g_module_verified = true;
}

std::uint32_t tick_vm(std::uint32_t max_cycles) noexcept {
    if (!g_module_verified || g_module_bytes.empty()) {
        return 0;
    }
    return run_bytecode(g_module_bytes, max_cycles).cycles_used;
}

}  // namespace gw::runtime::progs
