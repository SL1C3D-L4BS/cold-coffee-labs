#pragma once
// engine/runtime/progs_vm.hpp — bounded Greywater progs VM host (ADR-0122).
//
// `.gwprogs` v1 wire format: magic GWPG + u16 version + u32 word count + u16[] code.
// Verifier rejects truncated programs and unknown opcodes before execution.

#include <cstddef>
#include <cstdint>
#include <span>

namespace gw::runtime::progs {

enum class Opcode : std::uint16_t {
    Halt    = 0,  // stop; stack unchanged
    PushImm = 1,  // next two u16 = low, high of u32 payload (as int32_t)
    Add     = 2,  // pop b, a; push a+b
    Dup     = 3,  // duplicate TOS if present
    Pop     = 4,  // pop one value if present
};

enum class VerifyResult : std::uint8_t {
    Ok            = 0,
    TooSmall      = 1,
    BadMagic      = 2,
    BadVersion    = 3,
    SizeMismatch  = 4,
    TruncatedPush = 5,
    UnknownOpcode = 6,
};

[[nodiscard]] VerifyResult verify_bytecode(std::span<const std::byte> blob) noexcept;

struct RunResult {
    bool          ok{false};
    std::uint32_t cycles_used{0};
    std::int32_t  stack_top{0};
    bool          stack_empty{true};
};

/// Execute verified bytecode for at most `max_cycles` instructions (bounded work).
[[nodiscard]] RunResult run_bytecode(std::span<const std::byte> blob,
                                     std::uint32_t max_cycles) noexcept;

/// Gameplay tick hook: bounded slice; returns instruction cycles consumed.
[[nodiscard]] std::uint32_t tick_vm(std::uint32_t max_cycles) noexcept;

/// Optional: load a module for tick_vm (nullptr clears). Not thread-safe.
void set_active_bytecode(std::span<const std::byte> blob) noexcept;

}  // namespace gw::runtime::progs
