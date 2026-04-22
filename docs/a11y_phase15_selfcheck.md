# Phase 15 accessibility self-check — privacy & persistence UI

Use before merging RmlUi changes under `ui/privacy/`.

- [ ] Consent flow is screen-reader reachable via LocaleBridge strings and RmlUi accessibility hooks.
- [ ] Age gate uses keyboard-first navigation; tap targets ≥ 44×44 CSS px; respects `ui.reduce_motion`.
- [ ] Privacy prose targets Flesch Reading Ease ≥ 60 (see `tools/a11y/flesch_gate.py`).
- [ ] DSAR export is one primary action from the privacy screen (no deep nesting).
- [ ] “Delete My Data” requires explicit confirmation; progress is announced.
- [ ] Quota / conflict banners are non-flashing, dismissible, readable.
- [ ] Save slot list exposes row summaries for AT (slot id, name, timestamp, size).
- [ ] No audio-only consent path; audio cues have text + visual equivalents.
