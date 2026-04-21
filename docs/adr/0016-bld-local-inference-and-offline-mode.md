# ADR 0016 — Local inference & offline mode

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9F opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the `candle-core` + `llama-cpp-rs` pairing in `docs/02_SYSTEMS_AND_SIMULATIONS.md §13` (demoted — `llama-cpp-rs` becomes the fallback-of-fallback).
- **Superseded by:** —
- **Related:** ADR-0010 (provider abstraction); ADR-0013 (RAG — embeddings are local); `CLAUDE.md` #1 (RX 580 budget), #18 (no secrets into model context).

## 1. Context

Wave 9F (weeks 058–059) closes Phase 9 with local hybrid + offline-mode + structured output + Miri clean. Research deltas:

- **`mistral.rs` v0.8** — Rust-native LLM inference; built-in MCP client; **JSON-Schema-constrained structured output** (`Model::generate_structured`); CUDA/Metal/CPU backends; ships `mistralrs tune` for auto-tuning kernels to hardware. Reliability on small models (Qwen 2.5 Coder 7B Q4K) for structured tool calls has been demonstrably > 95 % in community benchmarks.
- `candle` — still a solid choice for **embeddings** (small encoder models). Not competitive vs. mistral.rs for the **generation** path at comparable quality at our Q4K budget.
- `llama-cpp-2` (newer binding than `llama-cpp-rs`) retains the `json_schema_to_grammar` → GBNF → `LlamaGrammar::parse` pipeline for platforms where mistral.rs cannot initialise.

## 2. Decision

### 2.1 Backend ordering (local half of ADR-0010's Router)

1. **mistral.rs — primary local.** Model: Qwen 2.5 Coder 7B, Q4K quantisation, 4 GB on-disk, ~6 GB VRAM peak on RX 580. `generate_structured` for every tool-call turn; free-text for chat turns.
2. **llama-cpp-2 — fallback of fallback.** Only reached when mistral.rs fails `init()` (no CUDA/Metal/CPU path). GBNF grammars compiled from tool schemas via `schemars::schema_for!` → `json_schema_to_grammar` → `LlamaGrammar::parse`. Correctness identical; perf lower.
3. **candle — embeddings only.** `nomic-embed-text-v2-moe` (see ADR-0013). Not used for generation.

### 2.2 Structured output contract

Every tool-call turn that targets a local provider emits a `CompletionRequest` with `structured_output: Some(tool_calls_schema)`. The schema is built at registry-build time from the tool catalogue:

```text
TopLevel {
  tool_calls: Vec<{
    tool_id: enum { "scene.create_entity" | "component.set" | ... },
    args: oneOf { ... per-tool arg schemas ... }
  }>,
  reasoning: Option<String>,
  is_final: bool
}
```

mistral.rs `generate_structured` constrains the decoder; the output is always a valid JSON per the schema. For the llama-cpp-2 path the schema is compiled to GBNF at provider init and cached.

Provider failures at schema-violation time: impossible by construction (constrained decoding can't emit invalid JSON); but in case of decoder internal error the response is translated to `ProviderError::Decoder` and the agent falls back per §2.1.

### 2.3 Offline mode

Offline mode is **not** silent degradation. It is an explicit state visible in the chat panel with a non-dismissable banner:

```
╔════════════════════════════════════════════════════════╗
║  OFFLINE MODE — cloud provider unreachable.            ║
║  Available: docs.search, project.find_refs, scene.query ║
║  Unavailable: build.*, runtime.*, code.apply_patch      ║
║                                                          ║
║  Reconnect and use /reconnect to return to full mode.   ║
╚════════════════════════════════════════════════════════╝
```

Triggered by:
- Any provider `health()` returning `Unreachable`.
- User command `/offline`.
- `.greywater/bld_config.toml` key `bld.mode = "offline"`.

While offline, the Router rejects any tool whose tier is `Execute`; `Mutate` is allowed if it has a pure-local implementation (`code.apply_patch` is one — it does not need the cloud; but the agent reasoning does, so if *only* the cloud is out we still disallow mutations; if the local model is available, mutations go through).

The banner is latched off only when a provider `health()` returns `Ok` again; there is no flicker.

### 2.4 Contract tests (shared across providers)

Already introduced in ADR-0010 §2.5. In 9F we **add local legs** to the contract suite:

```rust
#[test] fn contract_anthropic_cloud()     { /* gated by env */ }
#[test] fn contract_mistralrs_qwen_coder() { /* always runs on CUDA/Metal/CPU CI */ }
#[test] fn contract_llama_cpp_qwen_coder() { /* always runs */ }
```

Pass-rate gate: each provider ≥ 95 % on the 20-prompt suite. Any single-provider regression fails CI independently.

### 2.5 Cost benchmark

`cargo bench -p bld-provider --bench cost` runs the same 20-prompt suite against Claude Cloud (paid, env-gated) and mistral.rs local. Outputs a markdown report under `docs/daily/<date>-bld-cost.md` with totals. Acceptance gate for the 9F exit: local suite cost $0 with ≥ 70 % of Claude's pass rate. Higher is better; 70 % is the floor.

### 2.6 Miri discipline

`cargo +nightly miri test -p bld-ffi -p bld-bridge` runs nightly. Flags: `-Zmiri-strict-provenance -Zmiri-retag-fields`. Any UB in the boundary crates fails the nightly gate. We accept Miri's slower runtime (≈ 20× native); the nightly cadence is the trade-off.

All `unsafe` in `bld-ffi` is grouped into small, heavily-documented blocks per non-negotiable #5 spirit; each block has a `SAFETY:` comment that cites the invariant it relies on.

### 2.7 Auto-tune & on-first-run setup

`mistralrs tune` runs on first-run detection (no tuned kernels on disk). The user sees a one-time dialog: "First-run optimisation — this will take 2–5 minutes. Click Skip to run untuned (slower)." The tuning artefacts live at `.greywater/bld/mistralrs/cache/` and are machine-scoped (not committed).

## 3. Alternatives considered

- **Skip local for v1.** Rejected: offline mode is in the Phase 9 exit criteria; without local there is no offline.
- **Python child process for local inference.** Rejected: non-negotiable #4 (Rust only in BLD); also defeats the Rust single-runtime discipline.
- **Custom GGUF loader.** Rejected: reinvention of llama.cpp; no engineering value.

## 4. Consequences

- BLD works offline for read-heavy tasks.
- Local model runs constrained-JSON; no GBNF pipeline in the common path.
- Miri is the nightly UB gate on the C-ABI boundary.
- First-run latency is user-visible (tuning); documented.

## 5. Follow-ups

- `docs/02 §13` amended: `sqlite-vss → sqlite-vec`, `llama-cpp-rs → mistral.rs primary + llama-cpp-2 fallback`, `Nomic Embed Code → dual-path (prebuilt large + runtime small)`.
- `docs/04 §3` amended to reflect Rig as the carrier and the structured-output strategy.
- `docs/05 §Phase 9` row flips to `completed` on milestone sign-off.

## 6. References

- `mistral.rs` v0.8: <https://github.com/EricLBuehler/mistral.rs>
- `llama-cpp-2`: <https://docs.rs/llama-cpp-2>
- Qwen 2.5 Coder: <https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct>
- Miri: <https://github.com/rust-lang/miri>

## 7. Acceptance notes (2026-04-21, wave 9F close)

- `bld_provider::hybrid::HybridRouter` implements `ILLMProvider` and
  delegates across an ordered vector of `(priority, ILLMProvider)`
  entries. It reads its primary/fallback decision from the shared
  `OfflineFlag` and from each provider's `health()`:
  - Offline ⇒ only `ProviderKind::Local` candidates are considered.
  - Online ⇒ Cloud is preferred unless it reports `Unhealthy`, in
    which case the router falls through to Local and records the
    demotion in the audit stream.
  - `OfflineFlag` is toggled at runtime by the `/offline` slash
    command (see ADR-0015 §7) and surfaces to the editor via the
    session context.
- `MistralrsProvider` and `LlamaProvider` ship as feature-gated stubs
  (`--features local-mistralrs` / `--features local-llama`). The
  stubs advertise `ProviderHealth::Unhealthy { reason: "not
  configured" }` until a model path is attached, and return a
  `ProviderError::NotConfigured` on any call. This keeps the base
  `cargo test --workspace` lean while keeping the router's
  integration points real.
- Constrained JSON grammar, auto-tune cache wiring, and the
  benchmark harness per §2.4 / §2.5 / §2.7 stay gated behind the
  same local-inference features and are not activated in the
  default-feature CI matrix. They will land with wave 9G (model
  manager + first-run dialog).
- Miri gate: `bld-ffi`'s current exports are Miri-clean under
  `-Zmiri-strict-provenance -Zmiri-retag-fields`; the nightly job
  consumes the same target set described in §2.6.
