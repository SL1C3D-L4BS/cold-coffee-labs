# ADR 0025 — Engine console: dev overlay, command registry, debug-only gating

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11C)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0023 (event bus), ADR-0024 (CVars), ADR-0026 (RmlUi), blueprint §3.20

## Context

Every shipping runtime needs a dev console: tweak CVars, call commands, inspect state, reload assets. The console is **RmlUi-backed** per blueprint §3.20 and CLAUDE.md #12 (in-game UI is RmlUi, not ImGui).

## Decision

Ship `engine/console/` with:

- `ConsoleService` — RAII façade. Owns the command registry, a reference to `CVarRegistry`, a 256-line history ring, and an autocomplete trie over `{CVar names} ∪ {command names}`.
- `ICommand` — interface: `void exec(std::span<const std::string_view> argv, ConsoleWriter&)`, optional `complete(prefix, ConsoleWriter&)` for custom tab-completion.
- `ConsoleWriter` — scoped output handle; writes bytes to a ring that the RmlUi overlay drains each frame.
- Built-in commands (all flagged `kDevOnly` when appropriate): `help`, `set`, `get`, `bind`, `audio.duck_music`, `audio.stats`, `input.list_devices`, `ui.reload`, `fs.dump_asset_cache`, `time.scale`, `quit`, `events.stats`, `cvars.dump`, `bld.echo`.
- **Overlay**: a single `console.rml` document; drawn through the shared UI pass (ADR-0027). Toggle binds: backtick, F1, or gamepad Select+Back. The overlay document is *not* compiled into the binary — it is shipped as an asset under `assets/ui/console.rml`.

### Release-build gating

- Compile-flag `GW_CONSOLE_DEV_ONLY=1` (default ON in release) strips every `kDevOnly` command at link time via `[[no_unique_address]]` empty-tag optimisation.
- Release binaries ship with the console code-path compiled, but the command table is empty and the tilde-toggle is guarded behind the CVar `dev.console.allow_release` (default `false`).
- A "CHEATS ENABLED" banner shows whenever any `kCheat` CVar has deviated from its default, matching Valve-source-engine precedent.

### Accessibility

- The overlay respects `ui.text.scale` and `ui.contrast.boost` via the same RCSS theme used by the HUD (ADR-0026 §theming).
- The single text input has a 2-px `signal`-colour focus ring.
- Tab cycles: command line → history → close. Keyboard-only operation is first-class.

## Perf contract

| Gate | Target |
|---|---|
| Console open → first character rendered | ≤ 80 ms |
| `help` over 300 commands | ≤ 2 ms |
| Autocomplete 2-char prefix over 300 items | ≤ 0.5 ms |
| Heap on command dispatch | 0 B |

## Tests (≥ 8)

1. register / deregister commands; names are unique-or-error
2. argv parser handles quoted strings with embedded spaces
3. `set` type-coercion errors print the CVar's expected type
4. autocomplete by prefix and by substring, sorted stably
5. history persists across session via `user/console_history.txt`
6. release-build `kDevOnly` commands are absent from the command table
7. RmlUi data-model round-trip — writing to the model updates the service buffer and vice versa
8. concurrent publish from `ConsoleCommandExecuted` while a command is running does not reenter the command

## Alternatives considered

- **ImGui console** — violates CLAUDE.md #12. ImGui lives only in editor.
- **Terminal-attached REPL** — not usable on Steam Deck / console. UI-in-engine is the only portable answer.
- **Skip the release overlay entirely** — the `--allow-console` flag is a critical QA affordance during bug-bash; striping it out would cost us debug velocity in Phase 14+.

## Consequences

- `audio.*` and `input.*` domains gain one command each (`audio.stats`, `input.list_devices`) that dumps current state. These commands are the first BLD-readable console tap; Phase 18 wires them to the BLD Plan-Act-Verify loop as state-inspection hooks.
- The Phase-10 ad-hoc `std::fprintf(stderr, …)` calls inside `audio_service.cpp` are forbidden going forward; the audio service emits to `engine/core/logger` which the console can mirror.
