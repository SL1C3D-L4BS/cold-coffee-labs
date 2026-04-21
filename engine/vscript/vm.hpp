#pragma once
// engine/vscript/vm.hpp
// Phase 8 week 046 — stack-based VM that executes compiled vscript Programs.
//
// The VM is deliberately simple: one operand stack, one dispatch loop,
// allocation-free on steady state. It's the shipping runtime — cooked
// vscript blobs run on this; the interpreter (interpreter.hpp) stays as the
// reference oracle.
//
// API mirrors interpreter::execute() so callers (tests, editor preview)
// can flip between interpreter and VM without changing surface code.

#include "bytecode.hpp"
#include "interpreter.hpp"   // InputBindings, ExecResult, ExecFailure, ExecError

#include <expected>

namespace gw::vscript {

[[nodiscard]] std::expected<ExecResult, ExecFailure>
run(const Program& p, const InputBindings& inputs = {});

} // namespace gw::vscript
