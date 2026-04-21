# ADR 0014 — Plan-Act-Verify agent loop & iteration bounds

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9D opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** —
- **Superseded by:** —
- **Related:** ADR-0005 (CommandStack — every mutation reversible); ADR-0010 (provider); ADR-0012 (tool macro + dispatch); ADR-0015 (governance/audit — forward reference); `CLAUDE.md` #19 (phase-complete), #20 (doctrine first).

## 1. Context

Wave 9D (weeks 054–055) ships the agent loop itself — the brain that takes a user goal, plans a tool-call sequence, executes it, and verifies the outcome. Bare "let the LLM emit tool calls until it stops" does not satisfy any of our constraints: we need bounded cost, bounded wall-clock, reversible actions, and a failure mode that escalates to the user instead of looping silently.

Three well-known agent patterns:

1. **ReAct** — thought/action/observation loop. Good for exploration; weak on long plans.
2. **Plan-and-Execute** — one-shot plan, N-step execution. Fragile when the plan is wrong.
3. **Plan-Act-Verify** — one plan per turn, per-step verification, re-plan on failure. Best balance for our workload (file + scene + build + runtime + tests in one turn).

We pick Plan-Act-Verify as the primary and fall back to ReAct on verify failures.

## 2. Decision

### 2.1 Loop shape (primary)

```text
USER_MSG ─┐
          ▼
      ┌─ PLAN (call ILLMProvider with system-prompt + RAG-context + tool-catalog +
      │        few-shot exemplars) → returns Steps[] where each Step is
      │        { tool_id, args_json_template, why, verify_predicate }
      │
      ├─ ACT step k:
      │     1. Governance check (elicitation if needed)
      │     2. Registry::dispatch(step.tool_id, args, ctx)
      │     3. Audit record emitted
      │
      ├─ VERIFY step k:
      │     if step.verify_predicate != None:
      │         run the predicate (cheap check — see §2.3)
      │         if predicate fails:
      │             → fall to REACT sub-loop (see §2.2)
      │     else:
      │         proceed to next step
      │
      └─ until Steps exhausted OR budget exhausted OR user cancels
```

At every step boundary the chat panel streams the plan's next step as a collapsible `> thinking` block, so the user watches reasoning live. After execution, the user can click "expand" to see raw tool JSON in + out — nothing is hidden.

### 2.2 Fallback: ReAct sub-loop

When a step's verify fails, the agent enters a ReAct sub-loop with a **hard cap of 10 iterations** and a specialised prompt: "Your previous action failed verify with output X. Propose a single next tool call." On iteration 10 without success, the sub-loop exits, the original plan is abandoned, and the agent **escalates to the user** with a structured error: `"I tried N things; none worked. Here's what I tried and why each failed."`

### 2.3 Verify predicates

Predicates run cheaply (≤ 50 ms) and only check that the action's *intended effect* is visible. They are not correctness proofs. Examples, keyed by tool family:

| Tool family           | Verify predicate                                               |
|-----------------------|----------------------------------------------------------------|
| `scene.create_entity` | `gw_editor_find_entity(handle) != 0`                            |
| `component.set`       | `gw_editor_get_field(entity, path) == expected_value`           |
| `code.apply_patch`    | Parse the patch; check every `+` line is in file; every `-` line is not |
| `build.build_target`  | Exit code == 0; diagnostic list empty at severity >= error      |
| `vscript.compile`     | `vscript::Program` constructs without error                     |
| `runtime.play`        | `gw_editor_is_playing() == true`                                |
| `docs.search`         | No predicate (reads are self-verifying)                         |

The predicate is produced by the **planner** (the LLM) as part of the plan. If the LLM skips a predicate, the agent supplies a default from a builtin table (`default_verify_predicate(tool_id)`). Non-predicate'd steps still pass through the audit channel.

### 2.4 Step budget — hard caps

- **Plan size cap**: 30 steps. A plan > 30 is rejected and re-planned with the caveat "break this into smaller plans".
- **Total steps per turn**: 30 (plan) + 10 * fallback ReAct fires; effective ceiling ≈ 40.
- **Wall clock per turn**: 5 minutes. Exceed → cancel, audit `turn_timeout`, escalate.
- **Tool-call cost budget**: configurable in `.greywater/bld_config.toml` (default: no cloud-cost cap; warn at 50 calls).

Every budget overrun is an audit event and a chat-panel message to the user; the agent never silently swallows a retry.

### 2.5 Hot-reload flow (the *Brewed Logic* exit path)

Canonical flow for the scripted eval "Add a `HealthComponent` that ticks down 1 HP per second; apply to player; verify in PIE":

```
1. docs.search("HealthComponent existing definition") → empty
2. code.apply_patch(adds gameplay/src/health_component.hpp + .cpp)
   → elicitation (form mode: unified diff) → approve
3. build.build_target("gameplay_module")
   → verify: diagnostics empty
4. runtime.hot_reload("gameplay_module")
   → verify: module_generation() incremented
5. scene.set_selection([player])
6. component.HealthComponent.add(player, {hp: 100, tick_rate: 1.0})
   → verify: gw_editor_get_field(player, "HealthComponent.hp") == "100"
7. runtime.play()
8. runtime.step_frame(60)  # run 1 second at 60 FPS
9. runtime.sample_frame("HealthComponent.hp", player)
   → verify: 99 ≤ hp ≤ 100 (tolerance: tick may or may not have fired exactly)
10. runtime.stop()
```

Acceptance for wave 9D is this plan executing end-to-end with **at most one** form-mode elicitation (the diff), all mutations on the CommandStack (Ctrl+Z unrolls everything), and a `HealthComponent` hp reading that trips the predicate.

### 2.6 Concurrency

One agent turn = one `tokio::task` on the shared runtime (ADR-0011 §2.5). The task holds a `CancellationToken`; the chat panel's "Stop" button triggers it. Cancel during a tool call: the tool dispatcher awaits the cancel signal and returns `ToolError::Cancelled`; the CommandStack partial-state is rolled back.

Multi-session (9E) runs each session on an independent task; sessions do not share agent state — only the governance decision cache (`tier` approvals).

### 2.7 Determinism caveat

The LLM is non-deterministic (temperature > 0 on the brainstorm slash-commands; 0.0 elsewhere but still sampling). The *agent loop* is deterministic: same plan, same tool dispatch order, same verify predicates, same audit sequence. The eval harness (Phase 9 plan §5) asserts on end-state predicates (ECS queries, file contents) — not on the plan shape.

## 3. Alternatives considered

- **Pure ReAct.** Rejected: brittle for multi-step code-edit + build workflows; easy to loop.
- **Pure Plan-and-Execute.** Rejected: when a plan step fails, we have no local recovery.
- **Tree-of-Thought search.** Rejected for v1: burns 5–20× the token budget; revisit when token costs drop further.

## 4. Consequences

- One loop shape governs all agent turns. Easy to reason about, easy to audit.
- Step budgets are hard; no infinite loops in production.
- Failures escalate to the user, always.
- Hot-reload exit flow is the acceptance contract of the *Brewed Logic* milestone.

## 5. Follow-ups

- Phase 14 may introduce agent-driven multiplayer replication test generation — which will need the same loop with networking verify predicates.
- Phase 17 may add a `code_review_tool` that wraps the plan in a peer-review pass before execute.

## 6. References

- ReAct: Yao et al., 2022.
- Plan-and-Execute: LangChain community pattern.
- Claude agent patterns: Anthropic docs, 2026-03.

## 7. Acceptance notes (2026-04-21, wave 9D close)

- `bld-agent::composite::CompositeDispatcher` routes tool calls by
  prefix across family-specific dispatchers: `scene.*`/`component.*`
  via `BridgeDispatcher`, `rag.*` via `RagDispatcher`, plus the
  Phase-9D newcomers (`build.*`, `code.*`, `vscript.*`, `runtime.*`,
  `debug.*`) through their typed dispatchers in
  `bld-agent::build_tools`. Adding a new family is one-line.
- The `BuildExecutor`, `CodeHost`, `VscriptHost`, `RuntimeHost`, and
  `DebugHost` traits define the full editor surface the agent uses
  during Act; each has a deterministic `Mock*` implementation so
  Plan-Act-Verify can be exercised in unit tests without a running
  editor. `FsCodeHost` is a `SecretFilter`-gated filesystem-backed
  `CodeHost` so `code.write` and `code.patch` can't escape project
  root or whitelisted extensions.
- `ScriptedHotReload` in `bld-agent::build_tools` is the literal
  encoding of §2.5's acceptance script — `code.write` →
  `build.invoke` → `runtime.hot_reload` → `scene.set_selection` →
  `component.*.add` → `runtime.play`/`step_frame`/`sample_frame`/`stop`.
  The integration test drives it against mock hosts and asserts the
  per-step verify predicates plus an ordered audit trail.
- Step budget, verify-fail-budget, and re-plan cap are enforced in
  `Agent::run_turn`; exceeding any budget returns
  `AgentError::BudgetExhausted` and the CommandStack rollback fires
  on the next user `undo`.
