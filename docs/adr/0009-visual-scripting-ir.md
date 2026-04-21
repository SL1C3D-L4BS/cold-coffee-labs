# ADR 0009 — Visual Scripting: typed text IR, interpreter oracle, bytecode VM, AOT cook

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 8 weeks 045–047)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #14 (text IR is ground-truth for visual scripts), #5 (no raw `new`/`delete` on VM hot path), `docs/architecture/greywater-engine-core-architecture.md §Visual scripting UI`, `docs/architecture/grey-water-engine-blueprint.md §3.8`, `docs/05_ROADMAP_AND_MILESTONES.md §Phase 8 weeks 045–047`, ADR-0008 (for the `.gwvs.layout.json` sidecar pattern), ImNodes widget reference.

## 1. Context

Phase 8 week 045 introduces Greywater's visual scripting system. The requirements, pinned by prior doctrine:

- **The text IR is the source of truth.** Editors are views. This is how we get diff-friendly, merge-resolvable, AI-native scripts and avoid the Unreal-Blueprint failure mode where the on-disk asset is a binary node graph.
- **The node graph is drawn with ImNodes**, and the on-disk project carries a sidecar `.layout.json` for node positions. Positions **never** live in the IR.
- **Determinism and safety.** Scripts run at simulation rate — a compile that changes output for equivalent inputs is a regression; a VM that can heap-allocate per frame is a performance bug.
- **Two-tier runtime.** An interpreter (fast to author; also the golden oracle) and a bytecode VM (shipped runtime; AOT-cooked). They must agree bit-for-bit on the golden corpus.

The rest of this ADR defines the smallest viable shape that satisfies those constraints and can grow into Phase 9+ (loops, function calls, intrinsics, JIT) without on-disk re-breaks.

## 2. Decision

### 2.1 Module layout

```
engine/vscript/
  ir.hpp / ir.cpp                 // Type, Value, Pin, Node, Graph + validate()
  parser.hpp / parser.cpp         // text ↔ Graph round-trip
  interpreter.hpp / interpreter.cpp   // Phase 8 week 045 reference oracle
  bytecode.hpp / bytecode.cpp     // OpCode, Instr, Program + compile()
  vm.hpp / vm.cpp                 // Phase 8 week 046 stack VM — ships runtime
  CMakeLists.txt

editor/vscript/
  vscript_panel.hpp / vscript_panel.cpp   // Phase 8 week 047 ImNodes projection
```

The engine side has zero Vulkan / ImGui dependencies. The editor side owns its own `ImNodesContext` per panel so multiple `.gwvs` windows don't fight over globals.

### 2.2 IR shape (text format)

The text IR is the commit-diffable grammar:

```
graph <name> {
  input  <name> : <type> [= <literal>]
  output <name> : <type>
  node <id> : <kind> [<debug_name>] {
    in  <pin> : <type> [= <literal>]
    out <pin> : <type>
  }
  edge <src_id>.<src_pin> -> <dst_id>.<dst_pin>
}
```

Types are a closed enum: `int` (`int64`), `float` (`f64`), `bool`, `string`, `vec3`, `void` (control-only). Literals for `vec3` use `vec3(x, y, z)`.

**Rationale for enum types (not templates).** A closed type set lets the VM emit direct opcodes per type and lets the editor colour pins without walking a type tree. Generics land in a successor ADR when we have a use case; Phase 8 explicitly excludes them.

**Node kinds shipped in Phase 8:** `const`, `get_input`, `set_output`, `add`, `sub`, `mul`, `select`. Small on purpose — enough to prove the pipeline end-to-end without forcing us to commit to intrinsic shapes that Phase 9 will inform.

**Why `id` is an integer, not a UUID.** Within a single graph, ids are author-scoped names; the serializer preserves insertion order. Text round-trip tests (`serialize(parse(text)) == text`) depend on that stability.

### 2.3 Graph validation

`gw::vscript::validate(const Graph& g)` enforces:

- Every non-literal input pin is driven by exactly one edge.
- Every edge references valid `src_node.src_pin` and `dst_node.dst_pin`.
- Edge types agree (no implicit coercion in Phase 8).
- Graph is a DAG. Cycles are rejected (iterative DFS with a three-state colour array).
- Every `set_output` references a declared graph output by name, and its value pin type matches.

The interpreter and the compiler both call `validate()` before running. Making validation idempotent and fast (< 10 µs on a thousand-node graph) keeps it on the save/compile hot path without flinching.

### 2.4 Reference interpreter — the oracle

`gw::vscript::execute(const Graph&, const InputBindings&)`:

- Bottom-up demand-driven walk, memoised by `(node_id, pin_name)` so diamond DAGs don't recompute.
- Thread-local result cache reset per call so the steady-state hot path allocates zero on the second call onward.
- Type errors are surfaced as `ExecFailure { ExecError::TypeMismatch, message }` — no silent coercion.

**Why ship the interpreter?** Two reasons: (a) the editor preview button needs something that can run on a half-edited graph without the cook step, and (b) the VM golden corpus cross-checks against this implementation. If the VM and interpreter diverge on any input, the bug is in the compiler, period. See `tests/unit/vscript/vscript_vm_test.cpp`.

### 2.5 Bytecode + stack VM (Phase 8 week 046)

`Program` = `{ vector<Instr> code, vector<string> strings, vector<Vec3Value> vec3s, vector<IOPort> inputs, vector<IOPort> outputs }`.

`Instr` is a tagged-union record: 1-byte opcode + 7-byte padding + a union of `{ i64, f64, bool, idx }`. The size is 24 bytes (opcode + pad + the three-member union); larger than the minimum but we trade 8 bytes for branch-free operand decoding in the VM dispatcher.

**OpCodes shipped in Phase 8:**

| Op           | Code | Operand meaning                      | Net stack effect |
|--------------|------|--------------------------------------|------------------|
| `halt`       | 0    | —                                    | 0 (terminates)   |
| `push_i64`   | 1    | `i64`                                | +1               |
| `push_f64`   | 2    | `f64`                                | +1               |
| `push_bool`  | 3    | `b`                                  | +1               |
| `push_str`   | 4    | `idx` → `strings[]`                  | +1               |
| `push_vec3`  | 5    | `idx` → `vec3s[]`                    | +1               |
| `get_input`  | 6    | `idx` → `strings[]` (input name)     | +1               |
| `add`        | 7    | —                                    | −1               |
| `sub`        | 8    | —                                    | −1               |
| `mul`        | 9    | —                                    | −1               |
| `select`     | 10   | —                                    | −2               |
| `set_output` | 11   | `idx` → `strings[]` (output name)    | −1               |

**Compiler schedule.** Each `set_output` produces a sub-program: `emit_input(value_pin) ; set_output(name)`. `emit_input` follows drivers recursively, emitting pushes and operators. Shared sub-expressions *aren't* memoised in Phase 8 — they're re-evaluated. This is intentional:

1. The cost of memoisation in a stack machine is a scratch-slot bookkeeping pass that adds complexity for marginal speed on the hand-authored graphs shipping scripts use (seldom > 50 nodes).
2. Phase 9's JIT naturally solves the problem by emitting per-pin SSA values.
3. The oracle match against the interpreter catches any scheduling regression.

**VM hot path.** One `switch` over `op`, one `stack.emplace_back` or `stack.pop_back` per op. No allocation on steady state — the stack is `vector<Value>` with `reserve(code.size())` at entry. Phase 9 swaps `vector<Value>` for an aligned fixed-size array sized at compile time from the graph's known max-depth.

**Output completion.** On `halt`, the VM fills any output name not yet written with the declared default from `Program::outputs`. This mirrors the interpreter so a graph with a declared but unbound output still produces a complete result map.

### 2.6 Graph ↔ editor projection (ImNodes, week 047)

The editor's `VScriptPanel` is a pure view:

- Text buffer on the left = ground-truth.
- ImNodes canvas on the right = projection. On every "Parse" click (or on panel open) the graph is rebuilt from text; ImNodes node/pin ids are derived from `(node_id << 16 | pin_index << 1 | is_output)`.
- Layout is persisted to `<path>.layout.json` as `{"<node_id>": [x, y], ...}`. The file is written on save and read on load; a missing or malformed sidecar falls back to a default column grid.
- The panel cannot currently *edit* the graph through the node view — adding nodes, wiring pins, and renaming ports all happen via the text buffer. Phase 9 grows the ImNodes side to editable via `CommandStack`-backed operations.

**Sidecar format choice.** A hand-rolled ~40-line JSON reader/writer lives in `vscript_panel.cpp`. We explicitly chose not to pull `nlohmann::json` into the editor TU — the format is trivial, write-only from the editor, and read-mostly from the cook step (the cook never needs layout). If Phase 9 grows the editor to author project-wide structured data, we revisit.

### 2.7 AOT cook pipeline (outline)

Phase 8 week 046 ships the `compile` function from `Graph → Program`. The cook step that writes `Program` to disk (`.gwvsc`) is staged for Phase 9 alongside the rest of the asset cook manifest. The `Program` struct is designed to be trivially serializable (no owning pointers; strings are `std::string` whose bytes write out directly; `Vec3Value` is POD), so the cook is a `BinaryWriter` walk plus a small header. That lands under an `0009-follow-up` pull request when the cook manifest grows.

## 3. Consequences

### Immediate (this commit block — Phase 8 weeks 045–047)

- `engine/vscript/` module exists and is linked into `greywater_core`.
- Five dedicated test cases for the IR/validate/interpreter/parser (`vscript_ir_test.cpp`); five for the bytecode VM with oracle matching against the interpreter (`vscript_vm_test.cpp`). All green.
- `editor/vscript/vscript_panel.{hpp,cpp}` landed and docked in the editor's centre area.
- `ImNodesContext` is owned per-panel; the panel constructor creates one, the destructor destroys it.
- The panel's "Run" button executes the current graph through the interpreter and renders the output map.

### Short term (Phase 9)

- `.gwvsc` cook step serialises `Program` to disk; runtime loads bytecode directly without the parser on the hot path.
- Editable graph view: `CommandStack`-backed add-node, delete-node, wire-edge operations; the text buffer is kept in sync by re-serialising after each mutation.
- Additional opcodes: loops (`branch_if`, `jump`), function calls, native intrinsics (e.g. `glm::normalize`, math helpers). Each addition appends to the `OpCode` enum (never reorders) so cooked blobs stay binary-compatible.

### Long term

- JIT backend — lift `Program` to an SSA IR and emit native code via LLVM or Cranelift. Same `validate` front-end, same interpreter oracle.
- Behaviour-tree authoring uses this pipeline under the hood (per `docs/games/greywater/18_ECOSYSTEM_AI.md`).

### Alternatives considered

1. **Graph-as-ground-truth (Blueprint model).** Rejected — the diff/merge/AI-collaboration pain is well-documented. Text IR sidesteps the entire class of problems.
2. **Tree-walking interpreter as the ship runtime (skip the VM).** Rejected — allocation patterns and cache behaviour are unpredictable in a tree walker; stack VM lets us lock the operand stack to a fixed pre-sized array in Phase 9.
3. **Ship without `validate()`; trust the editor.** Rejected — any tool (CLI cook, BLD agent, raw text edits) that produces an IR goes through the same door; validate once, early, before the interpreter or compiler sees the graph.
4. **Per-node virtual `execute()` functions.** Rejected — forces every new node type to add a polymorphic class. The enum-dispatch + stack-machine approach keeps the node surface a data-only struct.
5. **Separate cook-time IR from author-time IR.** Rejected for Phase 8 — the text IR is the author-time IR and compiles directly to `Program`. If cook-time optimisations (constant folding, CSE) grow non-trivially, a separate lowered IR can land in Phase 9 without breaking `.gwvs` on-disk.

## 4. References

- `CLAUDE.md` non-negotiable #14 (text IR is ground-truth for visual scripts)
- `docs/architecture/greywater-engine-core-architecture.md §Visual scripting UI`
- `docs/architecture/grey-water-engine-blueprint.md §3.8`
- `docs/05_ROADMAP_AND_MILESTONES.md §Phase 8 weeks 045–047`
- ImNodes reference — <https://github.com/Nelarius/imnodes>
- Dear ImGui input-text callback API — <https://github.com/ocornut/imgui/blob/docking/imgui.h>

---

*Drafted by Claude Opus 4.7 on 2026-04-21 as part of the Phase 8 fullstack push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*
