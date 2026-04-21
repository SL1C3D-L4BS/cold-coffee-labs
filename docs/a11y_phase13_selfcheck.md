# docs/a11y_phase13_selfcheck.md — Phase 13 accessibility self-check

**Status:** Operational
**Scope:** Debug draw, editor BT/anim graph authoring, sandbox_living_scene HUD.

Every Phase-13 PR that introduces a new UI/debug surface runs through this checklist. Failure is an ADR-worthy escalation.

## 1. Colour-safe debug draw

- [ ] `animation_debug_pass` palette: skeleton = cyan (`#00B7FF`), IK goals = magenta (`#D62AB3`), active clip labels = white. No information is colour-only — shape (sphere = IK goal, line = bone) carries the semantic.
- [ ] `navmesh_debug_pass` palette: walkable polys = translucent green (`#30C36B80`), tile borders = yellow (`#F4C430`), path = solid orange (`#E96B2E`). Each poly also carries an integer id when `nav.debug.poly_ids` is on.
- [ ] All palettes verified against deuteranopia + tritanopia simulation (SIM Daltonism report stored under `docs/a11y/phase13_palette_report.png`).

## 2. Keyboard-first authoring

- [ ] Editor BT graph supports full keyboard nav: `Tab` cycles nodes, `Enter` edits properties, `Shift+Enter` opens vscript IR inspector, `F5` runs single-tick, `F10` step-next-tick, `F6` tracks blackboard. No behaviour is mouse-only.
- [ ] Editor anim graph: same contract. `Ctrl+P` hashes the current graph and copies `pose_hash` to clipboard.

## 3. Terminal/CLI output

- [ ] `sandbox_living_scene` prints `LIVING OK` / `LIVING DRIFT frame=<n> section=<PHYS|ANIM|BTRE|NAVQ>` to stdout. No ANSI colour relied on for semantics.
- [ ] `gw_runtime --replay --assert-living-hash` output is single-line, no screen redraws, Braille-reader friendly.

## 4. Sandbox HUD

- [ ] Labels scale with `ui.scale` CVar; default 1.0, tested at 2.0.
- [ ] All HUD labels printable via `ui.dump_text` for screen-reader capture.
- [ ] Focus indicator: when the sandbox window is focused, the HUD shows a high-contrast frame (`#FFFFFF` on `#000000`). Not colour-only — also 2-px outline.

## 5. Motion / flashing

- [ ] No debug overlay flashes faster than 2 Hz.
- [ ] Animation debug "selected bone" pulse ≤ 1 Hz, amplitude configurable via `anim.debug.pulse_rate`.

## 6. BLD-chat parity

- [ ] Every BLD-chat snippet that authors a BT or anim graph prints the resulting `content_hash` so it can be read aloud. Parity with editor export is enforced in `bt_parity_tests` / `anim_parity_tests`.

## 7. Audit cadence

Checklist reviewed at start of each Phase-13 wave (13A..13F) and at the end of Phase 13 as part of the roadmap closeout. Results logged in `docs/a11y/phase13_sweep_<date>.md`.
