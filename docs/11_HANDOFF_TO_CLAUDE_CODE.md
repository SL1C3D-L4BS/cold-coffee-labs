# 11_HANDOFF_TO_CLAUDE_CODE — Per-Session Protocol

**Status:** Operational
**Purpose:** Every coding session on Greywater_Engine, whether driven by a human engineer or by Claude Code, begins with this protocol. No exceptions.

---

## Cold-start checklist (10 minutes)

Run through these six steps in order. Do not start coding until all six are complete.

### 1. Read today's daily log
```
docs/daily/YYYY-MM-DD.md
```
If today's log does not exist, your `07_DAILY_TODO_GENERATOR.py` run missed a day — generate it now. Read what yesterday finished, what was blocked, what the *Tomorrow's Intention* said.

### 2. Read recent commits
```
git log --oneline -10
git status
```
Confirm the working tree is clean or intentionally dirty. Know what the last three commits did.

### 3. Re-read `CLAUDE.md`
```
./CLAUDE.md  (at project root)
```
The non-negotiables change subtly over time. The commitments do not. Re-read anyway.

### 4. Check Kanban state
```
docs/06_KANBAN_BOARD.md
```
Confirm:
- How many cards are In Progress? (WIP limit is 2)
- Which card is yours for the next session?
- Is anything stuck in Review?

### 5. Read the relevant numbered doc for the current phase
Check `docs/05_ROADMAP_AND_MILESTONES.md` for the current phase. Then read the numbered doc that matches:
- Phase 1–2 foundation work → `docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md`
- Rendering work → `docs/03_VULKAN_RESEARCH_GUIDE.md`
- C++/Rust patterns → `docs/04_LANGUAGE_RESEARCH_GUIDE.md`
- Systems questions → `docs/02_SYSTEMS_AND_SIMULATIONS.md`
- Architectural ambiguity → `docs/architecture/grey-water-engine-blueprint.md`

**Scope note.** Any deliverable listed under the active phase in `docs/05_ROADMAP_AND_MILESTONES.md`, in `docs/02_SYSTEMS_AND_SIMULATIONS.md` (Phase column), or — for Phases 1–3 — in `docs/10_PRE_DEVELOPMENT_MASTER_PLAN.md §3`, is **in scope for this session** if it hasn't landed yet. Do not filter by week row. The phase is the unit of delivery (`00` §9, `CLAUDE.md` non-negotiable #19).

### 6. Copy the day's card into the daily log
In `docs/daily/YYYY-MM-DD.md`, under *🎯 Today's Focus*, paste the Kanban card identifier and the *Context* field. You are now committed.

---

## CMake presets (canonical configurations)

Every Greywater build uses one of these presets. No ad-hoc `-D` flags in the IDE.

| Preset           | Config | OS      | Sanitizers | Purpose                                    |
| ---------------- | ------ | ------- | ---------- | ------------------------------------------ |
| `dev-win`        | Debug  | Windows | none       | Default developer workflow on Windows      |
| `dev-linux`      | Debug  | Linux   | none       | Default developer workflow on Linux        |
| `release-win`    | Release| Windows | none       | Release build on Windows                   |
| `release-linux`  | Release| Linux   | none       | Release build on Linux                     |
| `linux-asan`     | Debug  | Linux   | ASan       | AddressSanitizer run (nightly)             |
| `linux-ubsan`    | Debug  | Linux   | UBSan      | UndefinedBehaviorSanitizer run (nightly)   |
| `linux-tsan`     | Debug  | Linux   | TSan       | ThreadSanitizer run (nightly)              |
| `studio-linux`   | see preset | Linux | varies  | Phase 17 full shader/post stack; `phase17_budgets_perf`, `phase17_studio_renderer` |
| `studio-win`     | see preset | Windows | varies | Same on Windows |
| `ci`             | varies | both    | varies     | GitHub Actions matrix-driven               |

Usage:
```
cmake --preset dev-linux
cmake --build --preset dev-linux
ctest --preset dev-linux
```

### Rust-side (Cargo) presets

Cargo doesn't use CMake presets — it reads `rust-toolchain.toml` and `Cargo.toml`. The conventions are:

```
cargo build --release          # optimized BLD build
cargo test                     # unit tests
cargo test --release           # release-mode tests
cargo clippy -- -D warnings    # lint enforcement
cargo miri test -p bld-ffi     # UB check on the FFI boundary (nightly)
```

CMake invokes Cargo through Corrosion automatically during the main build — you only call Cargo directly when iterating on BLD.

---

## Hot commands (operational shortcuts)

```
# Full build (both C++ and Rust via Corrosion)
cmake --preset dev-linux && cmake --build --preset dev-linux

# BLD crate iteration (much faster for Rust-only changes)
cd bld && cargo build --release

# Run the sandbox
./build/dev-linux/sandbox/gw_sandbox

# Run the editor
./build/dev-linux/editor/gw_editor

# Regenerate pre-dated daily logs (one-time setup)
python docs/07_DAILY_TODO_GENERATOR.py

# Regenerate MCP tool schemas after adding a #[bld_tool]
cd bld && cargo build -p bld-tools

# Regenerate the C header for BLD after changing bld-ffi
cd bld && cargo build -p bld-bridge
```

---

## End-of-session protocol (5 minutes)

When the focus block ends:

### 1. Update the daily log
- Mark tasks completed under *📋 Tasks Completed*
- Note blockers under *🚧 Blockers*
- Write a one-sentence note under *🔮 Tomorrow's Intention*

### 2. Move the Kanban card
- If the card is done → to *Review*
- If still in progress → stays In Progress (don't exceed 2!)
- If blocked → to *Blocked* column with a reason

### 3. Commit
- Small commits. One logical change per commit. Follow Conventional Commits:
  ```
  feat(render): add timeline semaphore helper
  fix(ecs): resolve archetype-split determinism bug
  docs(blueprint): update §3.10 voice-chat latency budget
  ```
- CI runs on every push; do not break `main`.

### 4. If it is Sunday — weekly retro (30 min)
Read the seven most recent daily logs. Look for:
- What moved. What didn't.
- Which cards sat in Review too long.
- What is the *real* pace vs. planned pace for this phase.
- Is any Tier A work at risk? Escalate if so.

No coding during the weekly retro. This rule is load-bearing.

---

## If you get stuck

1. **Re-read `01_ENGINE_PHILOSOPHY.md`.** Half of all "I don't know what to do" moments dissolve once you remember the Brew Doctrine.
2. **Re-read `12_ENGINEERING_PRINCIPLES.md`.** The code-review playbook names most common mistakes; odds are your blocker is listed.
3. **Write an ADR.** If the problem is architectural ambiguity, draft `docs/adr/NNNN-decision-title.md` and make the trade-off explicit. Commit the ADR before committing code.
4. **Open BLD.** Ask the agent for context. `docs.search_engine_docs` is literally built for this.
5. **If it is Friday evening — stop.** Tomorrow is a rest day for a reason.

---

## What this document is not

- It is not the specification (`00`). It is not the philosophy (`01`). It is the *daily operational runbook*.
- It is not an onboarding guide. New engineers read the canonical set first.
- It is not a Kanban tutorial. `06` is.

---

*Read this file at the start of every session. Update it when the protocol changes. Ship by the protocol.*
