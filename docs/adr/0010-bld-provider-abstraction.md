# ADR 0010 — BLD provider abstraction & LLM selection policy

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 opening, wave 9A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** `CLAUDE.md` non-negotiables #15 (BLD separation), #18 (no secrets committed); `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §2.5; `docs/02_SYSTEMS_AND_SIMULATIONS.md §13`; `docs/04_LANGUAGE_RESEARCH_GUIDE.md §3`; `docs/architecture/grey-water-engine-blueprint.md §3.7`; ADR-0007 (C-ABI freeze); ADR-0016 (local inference — forward reference).

## 1. Context

Phase 9 wave 9A (weeks 048–049) exits on a Claude-streamed chat round-trip inside `gw_editor`. The foundational decision for every turn that follows is *what abstraction the agent talks to*. The wrong choice here costs us:

- Vendor lock-in to one LLM API on week one.
- A second rewrite in week 057 when the local-model path lands.
- An impossible contract-test story (different providers, different bugs, no shared harness).
- A secret-handling disaster if API keys leak through a generic `Display` or `Debug`.

Two days of 2026-04 research surfaced three live options that matter:

1. **Hand-rolled `reqwest` + `eventsource-stream`** per provider. Maximum control; ~1.5 kLoC duplicated across Anthropic / OpenAI / Gemini / local; brittle.
2. **`async-openai` + per-provider shim.** Tight for OpenAI + drop-ins; awkward for Anthropic's `/v1/messages` + tool-use format; no local story.
3. **`rig-core` (2026-03 stable).** Unified `LLMProvider` / `CompletionModel` traits over Anthropic / OpenAI / Gemini / Cohere / Perplexity; 5× memory reduction vs. Python wrappers; 25–44 % latency improvement on streaming; pluggable for local backends via the same trait.

Rig is also the only option that lets us **own** the `ILLMProvider` trait (wrapper pattern — if Rig diverges, we fall back to direct HTTP in ~2 days per provider; see risk R4 of the Phase 9 plan).

Separately, Anthropic's prompt-cache TTL dropped from 60 min to 5 min in 2026-Q1. Without a keep-alive pattern, cost savings collapse from 80 % to 40–55 % on long sessions. And `tool_search_tool` reduces context bloat 85 % for agents with 40+ tools — we will ship 79 at GA (per §2.2 plan), so it is mandatory, not optional.

## 2. Decision

### 2.1 The trait surface (owned by `bld-provider`)

BLD defines **one** trait, `ILLMProvider`, in `bld-provider/src/provider.rs`. Every concrete backend (Anthropic-via-Rig, OpenAI-via-Rig, mistral.rs local, llama-cpp-2 fallback) implements it. `bld-agent` and `bld-mcp` depend only on the trait. The trait surface (simplified):

```rust
#[async_trait::async_trait]
pub trait ILLMProvider: Send + Sync {
    fn id(&self) -> ProviderId;
    fn capabilities(&self) -> ProviderCapabilities;
    async fn complete(&self, req: CompletionRequest) -> Result<CompletionResponse, ProviderError>;
    async fn stream(&self, req: CompletionRequest) -> Result<BoxStream<'_, CompletionChunk>, ProviderError>;
    async fn embed(&self, req: EmbeddingRequest) -> Result<EmbeddingResponse, ProviderError>;
    async fn health(&self) -> ProviderHealth;
}
```

`CompletionRequest` carries the tool schema list, tool-use history, and a `structured_output: Option<schemars::Schema>` for JSON-Schema-constrained generation (9F). `CompletionChunk` is a sum type (`Token(String)` / `ToolCallStart{..}` / `ToolCallDelta{..}` / `ToolCallEnd` / `StopReason(..)`) so the UI can render partial state without provider-specific parsing.

### 2.2 Primary cloud provider — Claude Opus 4.7

`bld-provider/src/anthropic.rs` wraps `rig::providers::anthropic::Client` behind `ILLMProvider`. Default model `claude-opus-4-7`. Temperature 0.0 for agent turns; 0.7 is reserved for explicit brainstorm slash-commands (9E).

**Prompt-cache keep-alive (Claude TTL mitigation).** When a chat session is *active* (user has posted a message in the last 30 minutes and the tab is open), a 5-token no-op prompt is sent every **4 minutes** against the same prompt-cache prefix. This resets the cache TTL at ~4 % of a normal-turn cost, preserving the 80 %-cache-hit discount on the remaining session. A session config flag `keep_alive = false` disables it for cost-sensitive users. Not on by default for **background** sessions.

**Tool-search policy.** When `tool_schemas.len() > 20`, `tool_search_tool` is enabled on the Anthropic request. For shorter tool lists we rely on in-context tools (cheaper, no search round-trip). The threshold is a config constant `BLD_TOOL_SEARCH_THRESHOLD` defaulting to 20; revisitable via ADR amendment.

### 2.3 Secret handling

API keys are resolved through `keyring = "4"` (native backend per platform: `windows-native`, `linux-native` with sync-secret-service). `bld-provider::secrets` is the only module that loads a key; callers receive an opaque `ApiKey` newtype whose `Debug` and `Display` implementations render `"<redacted>"`. `Drop` zeroises the buffer. Logging is fine-tuned so the `anthropic` module can never emit a key even by accident: `tracing` field filters strip any span carrying `api_key = *`.

Keyring lookups follow a documented key-name policy: `bld/anthropic/api_key`, `bld/openai/api_key`, `bld/gemini/api_key`. `docs/BUILDING.md` documents the setup command per-OS.

### 2.4 Fallback provider ordering

Selection is made by `bld-provider::Router` (see ADR-0016 for the local half):

1. User-pinned provider (set in `.greywater/bld_config.toml`), if any.
2. `anthropic:claude-opus-4-7` — primary.
3. `mistralrs:qwen-coder-7b-q4k` — local primary (9F).
4. `llama-cpp-2:qwen-coder-7b-q4k-gbnf` — local fallback (9F).
5. `OfflineMode` — read-only tools only; agent responds "offline; please reconnect".

Selection rule for a given turn: walk the list; pick the first whose `health()` returns `Ok`. A `ProviderError::RateLimited` demotes the provider for 60 s.

### 2.5 Contract tests

Every implementation of `ILLMProvider` runs against a shared `provider_contract(p: impl ILLMProvider)` in `bld-provider/tests/contract.rs`. 20 prompts (8 completion / 6 tool-use / 3 streaming / 3 structured-output). Structural gates:

- Non-empty response on valid input.
- JSON-schema-valid tool calls when `structured_output` is set.
- Correct stop reason emitted.
- `embed` returns `(768,)` vectors for every provider that advertises the capability.

Pass rate < 95 % on any provider fails CI. Since this runs against real APIs, the cloud leg of the test is gated by an env var (`BLD_CONTRACT_TESTS_CLOUD=1`) to avoid burning credits on every commit; local legs always run.

## 3. Alternatives considered

- **Hand-rolled HTTP per provider.** Rejected: too much duplicated state-machine code for streaming, and Anthropic's `/v1/messages` format diverges enough from OpenAI's that a shared abstraction is worth paying for.
- **`llm`-chain crate.** Rejected: too opinionated on prompt templates; we want the chat-history and tool-use surface to be our shape, not a library's.
- **Only cloud in v1, local in a future phase.** Rejected: non-negotiable #18 forbids shipping anything that leaks secrets under failure, and we want offline-mode operable for `docs.search`, `scene.query`, and `code.read` on day one of 9A — defence-in-depth against an API outage during a demo.

## 4. Consequences

- One trait implementation per backend; replacements are local.
- Contract tests make regressions on any provider visible.
- Keep-alive pings add ~$0.01/hr/active-session; configurable off.
- Keyring becomes a required platform dep — documented in `BUILDING.md`.

## 5. Follow-ups

- ADR-0011 defines transport (MCP / elicitation) riding on top of the provider.
- ADR-0016 defines the local-inference half (mistral.rs primary, llama.cpp fallback).
- `docs/02 §13` and `docs/04 §3` amended to reflect Rig as the carrier and the revised `sqlite-vec` / `mistral.rs` decisions.

## 6. References

- Rig: <https://docs.rs/rig-core>
- Anthropic prompt caching & TTL: Anthropic API changelog, 2026-Q1.
- `tool_search_tool`: Anthropic API reference, 2026-03.
- `keyring-rs` 4.0.0-rc.3 platform features: <https://docs.rs/keyring>

## 7. Acceptance notes (2026-04-21, wave 9F close)

- `bld-provider` now exports four in-tree providers behind the same
  `ILLMProvider` trait: `ClaudeProvider` (default-features,
  HTTP-streaming against Anthropic `/v1/messages`), `MockProvider`
  (deterministic, always-on, the default in tests),
  `MistralrsProvider` and `LlamaProvider` (feature-gated stubs per
  ADR-0016 §7). Keystore lookup goes through `bld_provider::keystore`
  which wraps `keyring-rs` and falls back to env vars in headless CI.
- The `HybridRouter` added in wave 9F is *also* an `ILLMProvider`
  implementation, so every downstream consumer (agent loop, session
  manager, replay) depends on a single polymorphic handle regardless
  of whether the active backend is cloud, local, or a hybrid of both.
  This keeps §2 intact and makes offline-mode a runtime decision, not
  a wiring decision.
- Contract tests (`cargo test -p bld-provider`) cover the
  deterministic providers on every push; the `BLD_CONTRACT_TESTS_CLOUD=1`
  gate for the Claude leg is unchanged.
