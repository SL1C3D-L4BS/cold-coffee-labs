# ADR 0021 — Action map & accessibility (Wave 10E)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10E opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0020 (device layer), Game Accessibility Guidelines — Motor (Basic/Intermediate/Advanced), Hearing (Basic/Intermediate), General (Basic/Advanced).

## 1. Context

Wave 10E turns the device layer into a typed, rebindable, context-aware, accessibility-first action system. Accessibility is not a bolt-on — it drives the design of the action map itself. The cross-walk in §3 of this ADR is the Definition-of-Done for Phase 10 accessibility.

## 2. Decision

### 2.1 Action types

```cpp
template <typename T> class Action;

using ButtonAction = Action<bool>;
using AxisAction   = Action<float>;
using StickAction  = Action<math::Vec2f>;
using GyroAction   = Action<math::Vec3f>;
using EnumAction   = Action<u32>;
```

Each `Action<T>` owns:
- A human-readable `name` (stable across saves)
- A list of `Binding`s (primary + alternates)
- A `Processor` chain: `Deadzone → Curve → Invert → Scale → Clamp → Smooth`
- Optional accessibility flags: `hold_to_toggle`, `auto_repeat_ms`, `assist_window_ms`, `scan_index`, `can_be_invoked_while_paused`

### 2.2 Bindings

```cpp
struct Binding {
    BindingSource source;   // kbd/mouse/gamepad/gyro/hid
    u32           code;     // keycode / button / axis id
    ModifierMask  modifiers;
    float         threshold;   // for analog-as-button
    bool          inverted;
};
```

Composite bindings:
- **`Chord`** — N simultaneous bindings (`Ctrl+C`, `L1+R1`). Each chord is one `Action` input emitting `true` only while *all* members are held.
- **`Sequence`** — N sequential bindings with per-step `timeout_ms`. Useful for fighting-game inputs and accessibility "sticky sequence" replacements for chords.
- **`CompositeWASD`** — four Button bindings combined into a Vec2 axis. Implemented as a first-class binding kind so the processor chain runs once.

### 2.3 ActionSet and context stack

- `ActionSet` — named collection: `gameplay`, `menu`, `vehicle`, `editor`, `adaptive_default`, `one_button_simple`.
- At most **one** `ActionSet` is topmost at any time. A context stack enforces push/pop discipline (`InputService::push_set("menu")` / `pop_set()`).
- `consume` / `pass-through` semantics: an overlaid set may "eat" certain bindings (menu eats `toggle_menu`) while passing through others (radio hotkey always fires).

### 2.4 TOML persistence

Default map lives at `assets/input/default_gameplay.toml`. User overrides:

- **Windows**: `%APPDATA%\greywater\input.toml`
- **Linux**: `$XDG_CONFIG_HOME/greywater/input.toml` (fallback `~/.config/greywater/input.toml`)

Round-trip preserves unknown keys under `[extra]`. The schema:

```toml
[action.fire]
output = "bool"
bindings = [
  { device = "mouse",    input = "button_left" },
  { device = "gamepad",  input = "r2",        threshold = 0.3 },
  { device = "keyboard", input = "space",     modifier = "hold_to_toggle" },
]

[action.move]
output = "vec2"
processors = ["deadzone_radial(0.15)", "curve_quadratic", "clamp_unit"]
bindings = [
  { device = "gamepad",  input = "left_stick" },
  { device = "keyboard", input = "composite_wasd" },
]

[accessibility]
hold_to_toggle = true
repeat_cooldown_ms = 500
single_switch_scan_rate_ms = 900
enable_chord_substitution = true
```

Profiles:
- `input.toml` — active
- `input.$PROFILE.toml` — named profile; hot-swap via `InputService::load_profile`.

### 2.5 Rebind UX

- ImGui rebind panel in `editor/input_panel/` (Tier A, editor-only).
- In-game overlay using the temporary ImGui debug layer; replaced by RmlUi in Phase 11.
- Live capture: press any input → bound, with **conflict detection** (dialog: replace / swap / cancel).
- Xbox vs PlayStation prompts auto-swap based on last-used device type.
- Per-device alternate slots.

### 2.6 Accessibility defaults — built in, not opt-in

| Guideline | Tier | Implementation |
|---|---|---|
| Remappable/reconfigurable controls | Motor/Basic | §2.5 |
| Haptics toggle + slider | Motor/Basic | `haptics.toml` |
| Sensitivity slider per axis | Motor/Basic | `Processor::Scale` |
| All UI accessible with same input method as gameplay | Motor/Basic | `menu` set always bound to both kbd + gamepad |
| Alternatives to holding buttons | Motor/Intermediate | `hold_to_toggle` modifier: press converts to latched toggle; double-tap un-latches |
| Avoid button-mashing | Motor/Intermediate | `auto_repeat` modifier: single press treated as repeated input for configurable duration |
| Support multiple devices | Motor/Intermediate | Device fusion in `InputService` (no "primary device lock") |
| Very simple control schemes (switch / eye-tracking) | Motor/Advanced | `single_switch_scanner`: cycles through a configurable action subset on a timer; user triggers the single bound switch to activate current action. Preset `one_button_simple.toml`. |
| 0.5 s cool-down between inputs | Motor/Advanced | `repeat_cooldown_ms` global + per-action |
| Make precise timing inessential | General | `assist_window_ms`; `can_be_invoked_while_paused` |
| Stereo/mono toggle | Hearing/Intermediate | ADR-0018 §2.7 |
| Separate volume/mute per SFX/speech/music | Hearing/Basic | ADR-0019 §2.5 |
| Settings saved/remembered | General/Basic | TOML round-trip per profile |
| Settings saved to profiles | General/Advanced | multiple named `input.$PROFILE.toml` files |

### 2.7 Hold-to-toggle state machine

Every action flagged `hold_to_toggle = true` uses this transducer:

```
Idle --press--> Pressed
Pressed --release-fast--> Latched-on      (press < 250 ms → toggle on)
Pressed --release-slow--> Released        (press ≥ 250 ms → classic momentary)
Latched-on --double-tap--> Idle           (two presses within 350 ms → latch off)
Latched-on --single-tap--> Latched-on     (swallowed)
```

The transducer is stateless per-frame from the caller's perspective — the action outputs `true` continuously while latched. Idempotent under missed-press injection (interrupted frames).

### 2.8 Single-switch scanner

Cycles through a configurable list of N actions at `single_switch_scan_rate_ms` cadence. A single bound "scan_activate" button triggers whichever action is currently highlighted. Reachability: every action is reachable within `ceil(log2 N)` scan cycles when the scanner supports hierarchical groups; otherwise within N cycles linearly.

UI affordance (overlay, later replaced by RmlUi): highlight current action name on-screen; emit audio cue (TTS-ready in Phase 16).

### 2.9 Xbox Adaptive Controller / Quadstick auto-profile

On device connect, if `DeviceCaps::Adaptive` is set (MS VID 0x045E with Adaptive product IDs) or the device name matches the Quadstick fingerprint, the input service:

1. Sets `accessibility.adaptive_default = true`
2. Loads `input.adaptive_default.toml` on top of the active map
3. Enables hold-to-toggle globally
4. Turns on single-switch scanner for secondary menus

User can override via profile at any time.

## 3. Non-goals

- No eye-tracking hardware integration (Phase 16 a11y).
- No on-screen virtual keyboard (Phase 16).
- No voice-command input (Phase 14 voice + Phase 16 a11y).

## 4. Verification

- Replay-trace test: `.input_trace` → deterministic Action emission.
- Round-trip fuzz on TOML map: random bind, save, reload, compare.
- Hold-to-toggle property tests: idempotent under reordering of press/release pairs.
- Single-switch scanner reachability test: 1 000 synthetic hits, every action hit within expected cycles.
- Per-guideline self-check: `docs/a11y_phase10_selfcheck.md` table signed off before Phase-10 exit.

## 5. Consequences

Positive:
- Accessibility is enforced by the action-map type system, not policed by review.
- Profiles are data — QA can ship "Adaptive Controller preset" as a TOML without code.
- Rebind UX is editor-only in Phase 10; Phase 11 picks up RmlUi implementation for the shipping game.

Negative:
- TOML round-trip is strict — hand-edits that break syntax are rejected with a parse error (migration helper included).

## 6. Rollback

The whole action map can be bypassed by reading raw `RawSnapshot` from `InputService::snapshot()`. This is intentional — used by the editor's 3D camera which bypasses rebinding for debug navigation.
