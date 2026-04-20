# 06_KANBAN_BOARD — Greywater_Engine

**Status:** Operational
**Audience:** Whoever is about to pick up a card
**Enforcement:** WIP limit is not a suggestion

---

## Why Kanban

Greywater is likely to be built by a small team (founder + early hires) for much of the first year. Classic Scrum is overkill and induces ceremony fatigue. Kanban gives us visible state, strict WIP limits, and a single-task focus loop — everything we need, nothing we don't.

---

## Columns

Work flows left to right. A card never jumps columns. It never goes backwards without a note explaining why.

| Column         | Meaning                                                         | Limit |
| -------------- | --------------------------------------------------------------- | ----- |
| **Backlog**    | Identified work. Not yet triaged. Ideas, bugs, future features. | ∞     |
| **Triaged**    | Reviewed. Has Tier, Phase, Area, Type, Priority labels.         | 20    |
| **Next Sprint**| Pulled for the current week. Ready to start.                    | 6     |
| **In Progress**| Actively being worked.                                          | **2** |
| **Review**     | Code written; awaiting CI + **`12_ENGINEERING_PRINCIPLES.md` §J checklist** + external review. | 5     |
| **Blocked**    | Stuck on something external. Has a `Reason:` field.             | ∞     |
| **Documented** | Shipped; needs a change in a numbered doc before Done.          | 3     |
| **Done**       | Released. Never re-opened.                                      | ∞     |

**The In Progress limit of 2 is non-negotiable.** Exceeding it is the single most reliable predictor of burnout and scope slip. If a third card is "needed," first move one of the existing two to Blocked or Review.

**WIP is concurrency, not scope.** See `00_SPECIFICATION_Claude_Opus_4.7.md` §9.2 and `CLAUDE.md` non-negotiable #19. WIP≤2 limits *how many cards are actively being worked in parallel*. It does **not** limit *what enters the Triaged pool*.

---

## Phase entry — scope rule

**When a phase opens, every deliverable for that phase enters Triaged simultaneously.** The entries come from three authoritative sources, union-of:

1. Every row under that phase's section in `05_ROADMAP_AND_MILESTONES.md`.
2. Every row in `02_SYSTEMS_AND_SIMULATIONS.md` whose **Phase** column equals this phase and whose **Tier** is A or required-B.
3. For Phases 1–3, every item in the week-by-week expansion in `10_PRE_DEVELOPMENT_MASTER_PLAN.md §3`.

The phase milestone demo named in `05` is the single exit gate. No card is held "for a later week of this phase" — cards are ordered within the phase by priority, dependencies, and the puller's judgment, but all are in-scope from the phase-open moment.

If a card is genuinely out of scope for the current phase (e.g., accidentally filed here), it is moved to its proper phase in Triaged, not silently deferred.

---

## Labels

Every card carries five labels. Missing labels means the card is still in Backlog.

| Label       | Values                                                                      |
| ----------- | --------------------------------------------------------------------------- |
| **Tier**    | `Tier-A`, `Tier-B`, `Tier-C`, `Tier-G` (hardware-gated)                     |
| **Phase**   | `Phase-1` through `Phase-25` (see `05_ROADMAP_AND_MILESTONES.md`)          |
| **Area**    | `core`, `render`, `ecs`, `editor`, `bld`, `vscript`, `audio`, `net`, `physics`, `anim`, `input`, `ui`, `i18n`, `a11y`, `persist`, `telemetry`, `jobs`, `tools`, `docs`, `ci`, `meta` |
| **Type**    | `feat`, `fix`, `refactor`, `perf`, `docs`, `test`, `chore`, `adr`          |
| **Priority**| `P0` (breaks main), `P1` (blocks milestone), `P2` (normal), `P3` (nice)    |

---

## Pull policy

A card moves from **Triaged** to **Next Sprint** only when:
1. Its dependencies are in Done or Documented.
2. Its acceptance criteria are written and unambiguous.
3. Its priority justifies its position over anything else in Triaged.

A card moves from **Next Sprint** to **In Progress** only when:
1. The current In Progress count is `< 2`.
2. No card is stuck in Review waiting on the puller.
3. The puller has read `CLAUDE.md` and the relevant numbered doc.

**No card jumps Backlog → In Progress.** Triage is not optional.

---

## Card template

```
Title: <imperative verb, ≤70 chars>

Tier:     Tier-A | Tier-B | Tier-C | Tier-G
Phase:    Phase-N
Area:     <from Area list>
Type:     <from Type list>
Priority: P0 | P1 | P2 | P3

Dependencies:
  - <card ID or external blocker>

Estimate: <S | M | L | XL>  (S: <4h, M: 4–12h, L: 1–3d, XL: >3d — break it up)

Context:
  <1–3 sentences on why this work matters and what triggered it.
   Reference the relevant numbered doc or ADR.>

Acceptance criteria:
  - [ ] <specific, verifiable outcome>
  - [ ] <another>
  - [ ] Tests: unit | integration | smoke | perf-regression
  - [ ] Docs: none | 06_KANBAN_BOARD.md | <other numbered doc>

Files touched (expected):
  - engine/<subsystem>/...
  - tests/<subsystem>/...

Notes:
  <anything the next engineer needs to know that isn't in Context.>
```

---

## Daily ritual (10 minutes)

Done at the start of every coding session, before writing any code.

1. Read today's `daily/YYYY-MM-DD.md` log. Remember what stopped yesterday.
2. Check In Progress. Is either card blocked? Is either stale (>2 days with no progress)?
3. If In Progress is `< 2`, pull the highest-priority card from Next Sprint.
4. Paste the card identifier and Context into today's daily log under *🎯 Today's Focus*.
5. Triage dependencies. If a dependency just appeared, add it to the Backlog immediately.
6. Start a **90-minute focused block**. No Slack, no email, no Kanban-reshuffling. Just code.

---

## End-of-day ritual (5 minutes)

1. **Before moving a card to Review, run the `12_ENGINEERING_PRINCIPLES.md` §J checklist on your own diff.** Self-review is cheap; reviewer time is expensive.
2. Move the card:
   - Done with acceptance criteria + self-review? → Review.
   - Still working but stopping? → stays In Progress, note progress in the daily log.
   - Found it was blocked? → Blocked, with a `Reason:` field.
3. Update `daily/YYYY-MM-DD.md`:
   - *📋 Tasks Completed* — what shipped.
   - *🚧 Blockers* — what stopped.
   - *📝 Notes* — surprises, decisions, citations.
   - *🔮 Tomorrow's Intention* — one sentence.
4. Commit. Conventional Commits format (`feat(area):`, `fix(area):`, etc.).

---

## Weekly ritual (30 minutes, Sundays)

**No coding during this time.** The purpose is reflection and re-planning. If you code, you miss the point.

1. Read the last seven daily logs. Pattern-match: what actually happened vs. what was intended?
2. Scan Review. Anything stuck more than three days? Either land it or move it back to In Progress with a note.
3. Re-justify Next Sprint: is this week's pull still the right pull, given this week's findings?
4. Triage Backlog: promote high-signal items to Triaged, archive stale ones.
5. Check milestone progress against `05_ROADMAP_AND_MILESTONES.md`. If a phase gate is slipping, write an escalation note now, not later.

---

## Milestone tracker

Checkbox per milestone. Unticked until the gated demo exists as a recorded video.

- [ ] *First Light* (week 004 · Phase 1)
- [ ] *Foundations Set* (week 016 · Phase 3)
- [ ] *Foundation Renderer* (week 032 · Phase 5)
- [ ] *Editor v0.1* (week 042 · Phase 7)
- [ ] *Brewed Logic* (week 059 · Phase 9)
- [ ] *Playable Runtime* (week 071 · Phase 11)
- [ ] *Living Scene* (week 083 · Phase 13)
- [ ] *Two Client Night* (week 095 · Phase 14)
- [ ] *Ship Ready* (week 107 · Phase 16)
- [ ] *Studio Ready* (week 118 · Phase 18)
- [ ] *Infinite Seed* (week 126 · Phase 19)
- [ ] *First Planet* (week 134 · Phase 20)
- [ ] *First Launch* (week 142 · Phase 21)
- [ ] *Gather & Build* (week 148 · Phase 22)
- [ ] *Living World* (week 154 · Phase 23)
- [ ] *Release Candidate* (week 162 · Phase 24)

Tag the milestone in git when ticked:
```
git tag -a v0.1-editor -m "Editor v0.1 milestone"
git push origin v0.1-editor
```

---

## Anti-patterns (do not do these)

These are listed specifically because they will be tempting.

1. **Expanding WIP past 2 "because this one is small."** No. Two is two.
2. **Pulling a card before triage is complete.** You will waste time. Triage first.
3. **Skipping the weekly retro "because I'm on a roll."** Your roll is uninspected and probably wrong. Retro anyway.
4. **Letting Review pile up.** Review is not a parking lot. Five items max; clear it on Friday.
5. **Reclassifying a Tier A as Tier B "to hit the sprint."** The engine has to ship the Tier A set. If you're cutting Tier A, you're no longer shipping v1.0 — say so explicitly.
6. **Writing code without a card.** Every change traces to a card. "Just refactoring" is a card too.
7. **Committing without updating the daily log.** The log is the memory. Without it, context evaporates by morning.
8. **Working on Sunday before the retro.** You are entitled to two full rest days per month. Claim them.
9. **Holding a card more than 3 days without visible progress.** Split it. Ask for help. Move to Blocked. Don't silently sit.
10. **Skipping `clang-format`, `rustfmt`, or the CI matrix.** That is what reviewers waste time on. Run the tools before pushing.

---

## Escalation

If two or more of the following are true for a week, escalate to stakeholders:

- More than 3 cards in Blocked.
- Review has 5+ items for more than 48 hours.
- A Phase milestone is at risk (see `05_ROADMAP_AND_MILESTONES.md` escalation triggers).
- CI has been red for more than 24 hours.

Escalation is a Slack-or-equivalent message with subject line **"Greywater: Week N Escalation"**, linking the daily logs for context and naming the specific card(s) and the specific decision needed.

---

*Flow deliberately. Ship by the rhythm.*
