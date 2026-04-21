#pragma once
// engine/vscript/interpreter.hpp
// Phase 8 week 045 — reference interpreter for the Visual Scripting IR.
//
// The interpreter walks a validated Graph directly — no bytecode, no JIT.
// It exists for three reasons:
//
//   1. Editor previews. The node-graph panel (Wave 5) evaluates graphs live
//      so the user sees results without a separate cook step.
//   2. Test oracle. The Phase 8 bytecode VM (Wave 4) cross-checks its output
//      against the interpreter on a golden corpus — any divergence is a
//      compiler bug.
//   3. Debugger backbone. Stepping in the editor traverses the same
//      evaluation order that the interpreter uses.
//
// Execution model: bottom-up, memoised. For each graph-output port we
// recursively evaluate the driving edge, then every upstream dependency,
// caching each node's outputs by (node_id, pin_name) so diamond-shaped DAGs
// don't re-compute shared sub-expressions.
//
// Errors (ExecError) are raised on:
//   * type-mismatched operator inputs (validate() should have caught these —
//     this is a belt-and-braces check),
//   * integer division or select-with-wrong-type style runtime faults.
//
// The interpreter never allocates on the steady-state hot path: result
// tables are thread_local-reused across calls.

#include "ir.hpp"

#include <expected>
#include <string>
#include <unordered_map>

namespace gw::vscript {

enum class ExecError : std::uint8_t {
    ValidationFailed,
    UnknownInput,
    UnknownOutput,
    MissingDriver,
    TypeMismatch,
    UnsupportedNodeKind,
    InternalError,
};

[[nodiscard]] std::string_view to_string(ExecError e) noexcept;

// Inputs for a single execution: a map from graph.input.name → Value.
// Missing keys fall back to the IOPort's default `value` (populated by the
// parser from the `= literal` clause or, absent that, by default_value()).
using InputBindings = std::unordered_map<std::string, Value>;

// Executes `g` and returns the collected graph outputs by name. The call
// fails fast (ExecError + message) on the first error — partial results are
// not returned.
struct ExecResult {
    std::unordered_map<std::string, Value> outputs;
};

struct ExecFailure {
    ExecError   code = ExecError::InternalError;
    std::string message;
};

[[nodiscard]] std::expected<ExecResult, ExecFailure>
execute(const Graph& g, const InputBindings& inputs = {});

} // namespace gw::vscript
