# AGENTS.md — Cold Coffee Labs + ECC

This repo's **authoring surface is Cursor** (see `.cursor/`). The
Everything Claude Code (ECC) bundle is vendored under
`third_party/everything-claude-code/` and mirrored into
`.cursor/{agents,skills,commands,rules}`. Canonical upstream inventory:
**38 agents**, 156 skills, 72 commands, 89 rules (ADR-0119). Some older
briefs used “48 agents”; the **v1.10.0 source-of-truth is 38** first-class
agent markdown files; Cursor also exposes a separate **Task** subagent
menu (e.g. `generalPurpose`, `explore`, `code-reviewer`) with different
enums per harness.

Generic ECC instruction text: [`.cursor/AGENTS.md`](.cursor/AGENTS.md).

## Registered ECC agents (one line each)

*Planning & review*

| Slug | Role |
|------|------|
| `planner` | Decompose work into reviewable steps; dependency + risk list. |
| `architect` | System design, scaling, ADR-worthy tradeoffs. |
| `code-reviewer` | Post-edit quality pass for any language. |
| `tdd-guide` | Test-first and coverage discipline. |
| `security-reviewer` | Secret handling, trust boundaries, OWASP-style pass. |
| `refactor-cleaner` | Dead code, duplication, consolidation. |
| `doc-updater` | Keep docs/ codemaps aligned with diffs. |
| `docs-lookup` | Context7 / framework docs look-ups. |
| `performance-optimizer` | Hot paths, allocation, build/runtime perf. |
| `database-reviewer` | PostgreSQL, schema, query plans. |
| `e2e-runner` | Playwright-style user journeys, artifacts. |
| `build-error-resolver` | Generic build/type unblocking. |
| `harness-optimizer` | Tool budgets, model routing, agent harness health. |
| `loop-operator` | Long-lived loops, quarantine, hand-off. |
| `chief-of-staff` | Multi-inbox comms triage, drafts, follow-up. |
| `healthcare-reviewer` | Regulated / clinical safety (when those paths touch). |

*Language / stack reviewers*

| `cpp-reviewer` | C++ (Greywater / engine). |
| `rust-reviewer` | Rust, unsafe + concurrency. |
| `go-reviewer` | Go idiom, modules, pprof. |
| `typescript-reviewer` | TS/JS, Node, browser. |
| `python-reviewer` | Python, typing, security. |
| `java-reviewer` | Java / Spring. |
| `kotlin-reviewer` | Kotlin / KMP / Android. |
| `csharp-reviewer` | C# / .NET. |
| `flutter-reviewer` | Dart / Flutter. |

*Build resolvers*

| `cpp-build-resolver` | CMake, Ninja, `clang` errors. |
| `rust-build-resolver` | `cargo` / rustc failures. |
| `go-build-resolver` | `go` tool failures. |
| `java-build-resolver` | Maven / Gradle. |
| `kotlin-build-resolver` | Gradle, KSP. |
| `dart-build-resolver` | `flutter` / `dart` tool. |
| `pytorch-build-resolver` | Python ML stack, CUDA, training. |

*GAN / harness experiments*

| `gan-planner` | Spec → rubric for multi-agent eval. |
| `gan-generator` | Feature loop from evaluator feedback. |
| `gan-evaluator` | Playwright or harness scoring vs rubric. |

*Open source pipeline*

| `opensource-forker` | Sanitize and fork for release. |
| `opensource-sanitizer` | Secret / PII scan for OSS cut. |
| `opensource-packager` | CLA, README, templates. |

*Files live in* `.cursor/agents/<slug>.md` *and upstream*
`third_party/everything-claude-code/agents/`.

## When to delegate

- Cross-cutting feature → `planner` then `tdd-guide` then `code-reviewer`.
- C++ engine / `/engine` or `/gameplay` hot paths → `cpp-reviewer` and
  `performance-optimizer` for perf-sensitive diffs.
- Any auth, IPC, or player data → `security-reviewer`.
- **Never** use agents to skip ADR-0118 (read `graphify-out/GRAPH_REPORT.md` first) or
  ADR-0120 (AgentShield) requirements on guarded paths.
