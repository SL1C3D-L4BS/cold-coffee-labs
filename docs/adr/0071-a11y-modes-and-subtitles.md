# ADR 0071 — Accessibility modes + subtitles

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16E)
- **Companions:** 0072 (screen-reader bridge), 0073 (phase cross-cutting).

## 1. Decision

Ship `engine/a11y/` with four cross-cutting subsystems:

1. **Color-vision modes.** A 3×3 CAM16 LMS transform per mode applied in
   the tonemap pass. Modes: `None`, `Protanopia`, `Deuteranopia`,
   `Tritanopia`, `HighContrast`. The matrix is a `glm::mat3`; the UI
   pipeline reads `a11y::active_color_transform()` and the tonemap
   shader uses it as a uniform. The null/headless build still exposes
   the matrix so logic tests pass.

2. **Text scaling.** `ui.text.scale ∈ [0.5, 2.5]` drives every glyph's
   pixel size at render time. The settings binder writes it; layout
   reflow is automatic because glyph metrics are computed from the
   effective size. Minimum body size enforced ≥ 14pt per WCAG 2.2 AA
   §1.4.4.

3. **Motion reduction + photosensitivity.** `ui.reduce_motion` (bool) and
   `a11y.photosensitivity_safe` (bool). Both clamp flashing to ≤ 3 Hz
   per WCAG 2.2 AA §2.3.1 (three-flash rule) and compress large parallax
   translations > 32 px to ≤ 8 px in the UI shader.

4. **Subtitles + captions.** `engine/a11y/subtitles.{hpp,cpp}` queues
   caption events from audio + vscript + net, renders them via the UI
   service with `ui.caption.scale` and `ui.caption.speakercolor.enabled`,
   and exports a cue-list to telemetry so translators can audit
   per-locale timing.

## 2. CVars (new 16 — `a11y.*`)

| key | default | range |
|---|---|---|
| `a11y.color_mode` | `"none"` | one of {none, protan, deutan, tritan, hc} |
| `a11y.color_strength` | `1.0` | `[0, 1]` |
| `a11y.text.scale` | `1.0` | `[0.5, 2.5]` (mirrors `ui.text.scale`) |
| `a11y.contrast.boost` | `0.0` | `[0, 1]` |
| `a11y.reduce_motion` | `false` | |
| `a11y.photosensitivity_safe` | `false` | |
| `a11y.subtitle.enabled` | `true` | |
| `a11y.subtitle.scale` | `1.0` | `[0.5, 2.0]` |
| `a11y.subtitle.bg_opacity` | `0.7` | `[0, 1]` |
| `a11y.subtitle.speaker_color` | `true` | |
| `a11y.subtitle.max_lines` | `3` | `[1, 6]` |
| `a11y.screen_reader.enabled` | `false` | ADR-0072 |
| `a11y.screen_reader.verbosity` | `"terse"` | `terse`/`normal`/`verbose` |
| `a11y.focus.show_ring` | `true` | WCAG 2.2 AA §2.4.11 |
| `a11y.focus.ring_width_px` | `2.0` | `[1.0, 8.0]` |
| `a11y.wcag.version` | `"2.2"` | introspectable |

## 3. Color matrix freeze

```
Protanopia      Deuteranopia     Tritanopia       HighContrast
0.152  1.053 −0.205   0.367  0.861 −0.228   1.256 −0.077 −0.179   1.5  0   0
0.115  0.786  0.099   0.280  0.673  0.047   0.078  0.931 −0.009   0   1.5 0
−0.004 −0.048 1.052   −0.012 0.043  0.969  −0.007  0.018 0.990    0   0  1.5
```

Source: Machado, Oliveira, Fernandes (2009) CAM16 simulation, α=1.0. The
matrix is versioned; any future update is a new `a11y.color_mode` value
("protan2"), never a silent mutation. Canonical test fixture in
`tests/fixtures/a11y/color_matrices_v1.json`.

## 4. Subtitles pipeline

1. Audio world publishes `SubtitleCue{speaker, text_string_id,
   start_unix_ms, duration_ms, priority}` on the cross-subsystem bus.
2. `SubtitleQueue::push(cue)` enqueues; max concurrent lines = 3 by
   CVar. Overflow drops lowest priority.
3. `render()` walks active cues and emits a `ui::SubtitleLine` list that
   the UI service paints in a reserved bottom strip.
4. Telemetry records `a11y.subtitle.emitted` with speaker + duration
   (PII-scrubbed) when `tele.events.enabled`.

## 5. Exit gate

`a11y::selfcheck()` returns a `SelfCheckReport` with pass/fail booleans
per WCAG 2.2 AA criterion that's statically checkable (contrast, text
size, focus visibility, captions available). The *Ship Ready* demo
consumes this and prints `a11y_selfcheck=green` on all-pass.
