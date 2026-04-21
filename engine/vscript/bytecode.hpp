#pragma once
// engine/vscript/bytecode.hpp
// Phase 8 week 046 — Ahead-of-time-cookable bytecode for visual scripts.
//
// The bytecode is the ship format: once a graph is cooked, shipping builds
// never parse text. The interpreter from `interpreter.hpp` stays as the
// reference oracle — the VM here must match it bit-for-bit on the golden
// corpus (see tests/unit/vscript/vscript_vm_test.cpp).
//
// Design:
//   * Stack machine (32-bit op + optional 64-bit operand), easy to JIT later
//     and trivial to interpret without branch mispredictions dominating.
//   * Constant pool for strings + vec3 literals; scalar literals are
//     immediate operands to keep the hot path tight.
//   * No allocations during VM execution — operand stack is a fixed array
//     sized to graph depth at cook time.

#include "ir.hpp"

#include <array>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace gw::vscript {

// 8-bit opcode. Leaving head-room for Phase 9 additions (loops, function
// calls, native intrinsics); never rearrange the prefix — saved .gwvsc
// blobs are keyed by opcode index.
enum class OpCode : std::uint8_t {
    Halt       = 0,
    PushI64    = 1,
    PushF64    = 2,
    PushBool   = 3,
    PushStr    = 4,   // operand = constants[] index
    PushVec3   = 5,   // operand = constants[] index
    GetInput   = 6,   // operand = names[] index (graph input name)
    Add        = 7,
    Sub        = 8,
    Mul        = 9,
    Select     = 10,
    SetOutput  = 11,  // operand = names[] index (graph output name)
};

[[nodiscard]] std::string_view opcode_name(OpCode op) noexcept;

// A single instruction — opcode + one 64-bit operand (interpreted per op).
// Union-like payload: the VM reads the member matching the opcode.
struct Instr {
    OpCode        op        = OpCode::Halt;
    std::uint8_t  _pad[7]   = {};
    std::int64_t  i64       = 0;   // PushI64
    double        f64       = 0.0; // PushF64
    std::uint8_t  b         = 0;   // PushBool
    std::uint32_t idx       = 0;   // PushStr / PushVec3 / GetInput / SetOutput
};

// Compiled program — bytecode + constant pools + metadata for the VM.
struct Program {
    std::vector<Instr>       code;
    std::vector<std::string> strings;   // constant strings + names
    std::vector<Vec3Value>   vec3s;     // vec3 literal pool
    // Graph interface copies — the VM needs to know output names + types so
    // the result map can be populated without re-consulting the Graph (the
    // Graph may not be in memory in a cooked build).
    std::vector<IOPort>      inputs;
    std::vector<IOPort>      outputs;

    // Index of the name in `strings`; -1 if not found.
    [[nodiscard]] std::int32_t find_name(std::string_view s) const noexcept;
};

enum class CompileError : std::uint8_t {
    ValidationFailed,
    UnsupportedNodeKind,
    InternalError,
};

struct CompileFailure {
    CompileError code = CompileError::InternalError;
    std::string  message;
};

// Translate a (validated) Graph to a Program. Returns a CompileFailure if
// the graph is malformed or uses an unsupported node kind.
[[nodiscard]] std::expected<Program, CompileFailure>
compile(const Graph& g);

} // namespace gw::vscript
