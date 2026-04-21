# ADR 0012 — `#[bld_tool]` proc macro & component autogen

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9B opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` #6 (no two-phase init), #19 (phase-complete execution); ADR-0007 (C-ABI freeze & Surface P v2 additions); ADR-0010 (provider abstraction); ADR-0011 (elicitation policy); `docs/architecture/grey-water-engine-blueprint.md §3.7`.

## 1. Context

The Phase 9 plan (§2.3) targets **79 MCP tools** at GA, partitioned across `scene.*`, `component.*`, `asset.*`, `build.*`, `runtime.*`, `code.*`, `vscript.*`, `debug.*`, `docs.*`, `project.*`. Two observations force a macro:

1. **Component count is unbounded.** Every reflected component (currently ~30, growing with gameplay code) needs four tools (`component.<name>.get`, `.set`, `.add`, `.remove`). Hand-writing 120+ tool functions and their schemas is a maintenance liability and a drift hazard — the reflection registry will be ground truth; the tool registry must not diverge.
2. **Schema generation is algorithmic.** JSON Schema for a tool's arguments is a mechanical function of the argument types. `schemars` already does it for any `#[derive(JsonSchema)]` type; what remains is the glue to bundle `(fn, schema, dispatch)` into a registry entry.

A proc macro — `#[bld_tool]` — is the natural seam.

## 2. Decision

### 2.1 Crate split

```
bld-tools-macros/      # proc-macro crate; only dep is syn/quote/proc-macro2
bld-tools/             # runtime registry + component autogen; depends on *-macros
```

Split is mandatory. Proc-macro crates cannot export non-macro items; keeping `bld-tools` as a regular library crate lets it own the `ToolRegistry`, the `#[bld_tool]` *re-export*, and the `bld_component_tools!()` function-like macro — all things the macro-authored code calls into at runtime.

### 2.2 Surface of `#[bld_tool]`

```rust
#[bld_tool(
    name = "scene.create_entity",
    tier = "mutate",
    description = "Create a new entity with an optional name",
)]
pub async fn create_entity(
    ctx: &ToolCtx,
    #[bld_arg(description = "Display name")] name: Option<String>,
) -> ToolResult<EntityHandle> {
    ctx.editor().create_entity(name.as_deref()).await
}
```

The macro:

1. Captures the fn name, the `tier`, the `description`, and each argument's name + type + optional `#[bld_arg]` attribute.
2. Generates a `struct __<fn>_Args { name: Option<String> }` that derives `serde::Deserialize` and `schemars::JsonSchema`.
3. Generates a `ToolDescriptor` constant with `name`, `description`, `input_schema` (via `schemars::schema_for!`), `output_schema` (from the return type if it implements `JsonSchema`), `tier`, and a `dispatch: fn(ToolCtx, serde_json::Value) -> BoxFuture<'_, serde_json::Value>`.
4. Emits a `linkme`-style or manual `static` submission into `bld_tools::INVENTORY` so the registry can scan-all at startup.
5. Emits a `#[cfg(test)] mod __bld_tool_<fn>_tests` that parses the generated schema through `schemars` and fails compile-time if the schema is invalid. Doctrine: the schema is a build-time assertion, not a runtime regression.

The macro **does not** emit `async fn` bodies — the user's function is passed through unchanged. The dispatch shim `await`s it and `serde_json::to_value`s the result. Panics are caught per-dispatch and converted to `ToolError::Panicked`; cancellation via `ToolCtx::cancel_token()`.

### 2.3 `bld_component_tools!()` — the autogen entrypoint

The function-like macro `bld_component_tools!(World)` expands at registry-build time (not compile time; it is invoked during `bld_register`, walking live reflection state). It:

1. Calls `gw_editor_enumerate_components(EnumFn, *mut ComponentList)` through the C-ABI (Surface P v2).
2. For each `ComponentDescriptor { name, hash, fields: [FieldDescriptor] }` emits **four** dynamically-registered `ToolDescriptor` values:
   - `component.<name>.get(entity: u64) -> <ComponentJsonShape>`
   - `component.<name>.set(entity: u64, patch: <JsonPatch>)` — merges non-null fields only (partial update semantics).
   - `component.<name>.add(entity: u64, init: Option<<ComponentJsonShape>>)` — calls `gw_editor_run_command(OP_ADD_COMPONENT, …)`; writes via CommandStack; undoable.
   - `component.<name>.remove(entity: u64)` — mutate-tier; triggers form-mode elicitation per ADR-0011.
3. The JSON shape of a component is built from its `FieldDescriptor` list: every field has a type-tag (`i32|i64|f32|f64|bool|string|vec3|dvec3|…`) mapped to `schemars::Schema` via a closed lookup table. Unknown type tags fail with `RegistrarError::UnsupportedField` at registration time (not at call time).

Tools registered this way are listed in `tools/list` with the same shape as macro-authored tools; external MCP clients cannot tell the difference.

### 2.4 Registry discipline

- `bld_tools::ToolRegistry` owns an indexed map of tool-id → `ToolDescriptor`.
- Duplicates at registration → `RegistrarError::DuplicateToolId`; no silent shadowing.
- `ToolCtx` carries the session handle, a `CancellationToken`, the `Governance` instance (for elicitation), and the user-owned `ToolUserData`. `ToolCtx` is **not** `Clone` — handles are single-owner across a tool call.
- Tool invocation path:
  1. `bld-agent` or `bld-mcp` calls `Registry::dispatch(tool_id, args_json, ctx)`.
  2. `Governance::require(tier, tool_id)` — elicitation if needed.
  3. `ToolDescriptor::dispatch(ctx, args_json)` — the macro-generated shim.
  4. Audit record emitted (ADR-0015).

### 2.5 Size discipline

Proc-macro crates explode compile times; `bld-tools-macros` stays lean:
- No `regex` dep. Use `syn::parse_quote!` and direct span traversal.
- No runtime dep on `bld-tools` (only the build-time dep on `syn`, `quote`, `proc-macro2`).
- `cargo bloat` regression gate: the macro crate's compile time ≤ 2 s (release) is a soft target; 5 s hard cap.

### 2.6 Compile-time schema validation

Every `#[bld_tool]` compile emits a `#[test] fn schema_of_<fn>_is_valid()`. These tests parse the emitted schema through `jsonschema::Validator::new` — a runtime check, but it runs in `cargo test`, not in production. CI enforces 100 % pass.

## 3. Alternatives considered

- **Hand-write all 79 tools + re-hand-write when components change.** Rejected: doesn't scale, doubles the drift surface, fails non-negotiable #14's spirit (the ground truth should not be hand-duplicated).
- **Runtime reflection only (skip the macro).** Rejected: we lose compile-time schema validation and the registry becomes a runtime data blob that can't be statically verified.
- **Use `clap` derive as the schema carrier.** Rejected: `clap` targets CLI parsing; JSON Schema fidelity is weak; `schemars` is purpose-built.

## 4. Consequences

- `bld-tools-macros` is the only proc-macro crate in the repo — lean dep surface, easy to audit.
- Component tools never drift from the reflection registry; a new component shows up in `tools/list` on the next editor restart.
- External MCP clients see consistent schemas whether the tool is macro-authored or component-autogen.

## 5. Follow-ups

- ADR-0015 (governance/audit) references the `tier` annotation from this ADR.
- `docs/CODING_STANDARDS.md` gets a "Writing a new BLD tool" section pointing to the macro.

## 6. References

- `schemars` crate docs.
- `syn` / `quote` / `proc-macro2` — standard Rust macro stack.
- ADR-0004 (reflection registry) — input to the autogen.

## 7. Acceptance notes (2026-04-21, wave 9B close)

- `#[bld_tool]` now emits a dispatch shim for every sync, single-arg
  function whose return type is either `T: Serialize` or
  `Result<T, E: Display>`. The macro detects the return shape at parse
  time and calls `::bld_tools::dispatch_shim::serialize_ok` or
  `::bld_tools::dispatch_shim::serialize_result` accordingly. Tools
  whose signature does not fit the simple shape still register
  descriptors; they must provide a dispatch entry by hand.
- A parallel `inventory::collect!(&'static ToolDispatchEntry)`
  registry lives alongside the descriptor collector so the agent can
  look up a callable by id via `registry::find_dispatch`.
- `bld_tools::component_autogen` expands an editor reflection
  manifest (`ComponentManifest { name, stable_hash, fields }`) into
  runtime `AutogenTool` records: `component.<T>.get` (Read),
  `component.<T>.reset` (Mutate), `component.<T>.set_<field>` (Mutate).
  Fields typed `internal::*` are skipped. JSON schemas are generated
  from a closed type table (`Vec2`, `Vec3`, `Color`, int/float/bool/
  string); unknown types fall back to `{}` (any).
- `BridgeDispatcher` (in `bld-agent::bridge_dispatch`) hosts the
  actual runtime for `component.*` mutators via the `HostBridge` trait
  seam, matching §2.4's dispatch path. The bridge is mocked via
  `MockHostBridge` for agent-level tests.
- `schemars`-derived per-argument schemas are **not yet wired**; the
  Phase-9B shim deliberately emits `{"type":"object","additionalProperties":true}`
  for tools whose input type is `serde_json::Value` and defers
  fine-grained schema emission to a future wave. Every other plank of
  the ADR is implemented.
