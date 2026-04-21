# docs/a11y_phase14_selfcheck.md — Phase 14 accessibility self-check

**Status:** Operational
**Scope:** Voice chat, session discovery UI, net debug overlays, `sandbox_netplay` HUD.

Every Phase-14 PR that introduces a new UI or debug surface runs through this checklist. Failure is an ADR-worthy escalation.

## 1. Voice chat opt-in & control

- [x] Default voice mode is **push-to-talk** (`net.voice.push_to_talk = true`). Voice-activity mode is opt-in behind a settings toggle.
- [x] "Mute all remote voice" is a single keybind (default `F7`) and a single UI click; state is audibly confirmed by a short 440 Hz chirp.
- [x] Per-peer mute is tab-reachable from the session HUD and persists across runs via `engine/core/config` (`net.voice.mute_mask`).
- [x] Mic indicator draws a static ring when push-to-talk is **not** pressed and a 2 Hz maximum pulse when transmitting. Pulse respects `ui.reduce_motion`.
- [x] No voice-only ping exists — every voice gameplay cue (e.g. "teammate speaking") also surfaces as a text log line.

## 2. STT caption hook

- [x] `IVoiceTranscriber` interface exposes an `on_voice_frame(PeerId, std::span<const float>) → std::string_view` hook; the default implementation is a no-op, and the Phase 16 backend plugs in without changes to the voice hot path.
- [x] Caption text, when produced, flows through the HUD's subtitle sink (Phase 16 Ship Ready). Line length caps at 38 chars default; speaker name is prepended.
- [x] Captions are sticky for 2 s after the last frame of speech, so deaf and HoH players don't chase the end of a phrase.

## 3. Color-blind-safe debug overlays

- [x] `net.debug` overlay shares the 8-entry palette used by `physics.debug` + `nav.debug`. No information is color-only — shape or icon carries the meaning.
  - Server ↔ client arrows: triangle end-cap (client) vs. square end-cap (server).
  - Packet-loss indicator: outline glow (≤ 2 Hz pulse, amplitude-capped) + numeric "loss=X %" label.
  - Latency indicator: bar chart with 4-tick axis + numeric "rtt=XX ms".
- [x] Deuteranopia + tritanopia simulation verified (SIM Daltonism report stored under `docs/a11y/phase14_palette_report.png`).
- [x] Desync alert strip: bright red **and** thick diagonal stripe texture so it is unmistakable without color.

## 4. Terminal / CLI output

- [x] `sandbox_netplay --role=linkroom` emits one `NETPLAY OK` or `NETPLAY DRIFT frame=<n> section=<REP|PRE|VOI|SES>` line to stdout — no screen redraws, Braille-reader friendly.
- [x] `net.stats` prints a single wide line with all stats; column headers included. No ANSI color relied on for semantics.
- [x] `.desync.bin` diff dump has a companion `.desync.txt` human-readable summary emitted by `net.desync.dump`.

## 5. Keyboard-first authoring / operator

- [x] Session HUD: `Tab` cycles controls, `Enter` selects, `Shift+Enter` opens session details, `F9` joins-by-code. No mouse-only action.
- [x] Join-by-session-code dialog: session codes are printable ASCII, copy/paste works into screen readers; no glyph-only session identifiers.
- [x] `net.linksim` console command has long-form tab-completion identical to short-form so muscle memory works both ways.

## 6. Motion / flashing

- [x] No debug overlay flashes faster than 2 Hz.
- [x] Packet-loss and desync indicators pulse ≤ 2 Hz, amplitude configurable via `net.debug.pulse_rate`.
- [x] Connecting/disconnecting banner fades at 1 Hz max; respects `ui.reduce_motion` (hard cut when enabled).

## 7. BLD-chat parity

- [x] BLD-chat snippets that author session descriptors or replicate-component decls print the resulting `content_hash` so it can be read aloud. Parity with editor export enforced in `net_session_parity_test`.

## 8. Audit cadence

Checklist reviewed at start of each Phase-14 wave (14A..14L) and at the end of Phase 14 as part of the roadmap closeout. Results logged in `docs/a11y/phase14_sweep_<date>.md`.
