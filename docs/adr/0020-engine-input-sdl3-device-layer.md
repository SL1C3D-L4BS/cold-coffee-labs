# ADR 0020 — Input: SDL3 device layer (Wave 10D)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 10 Wave 10D opening)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** `CLAUDE.md` #11 (OS headers quarantined), ADR-0021 (action map), ADR-0022

## 1. Context

Phase 2 seeded an `engine/platform/input.cpp` stub that carried only keyboard/mouse structs. Phase 10 needs a full device-capability layer: gamepads, HID (flight sticks, HOTAS, Xbox Adaptive Controller, Quadstick), haptics, sensors, with stable IDs across hotplug and deterministic replay for CI. SDL3 (3.4.0, 2026-01-01) is the smallest credible cross-platform surface that handles all of the above with first-class hotplug and the 2025 DualSense / trigger-haptics API.

## 2. Decision

### 2.1 Layering

```
engine/input/                           (OS-free)
  input_types.hpp         — DeviceId, DeviceCaps, KeyCode, GamepadButton, axes
  input_backend.hpp       — IInputBackend interface
  input_trace.hpp/.cpp    — record/replay .input_trace for CI
  device.hpp/.cpp         — Device, DeviceRegistry, capability probe
  haptics.hpp/.cpp        — cross-device rumble + trigger feedback
  input_service.hpp/.cpp  — RAII façade (poll, snapshot, events)

engine/platform/input/                  (only TUs that touch SDL)
  sdl3_backend.cpp        — SDL3 implementation of IInputBackend
  sdl3_haptics.cpp
```

The existing `engine/platform/input.cpp` (Phase 2 stub) is promoted to a thin include shim so the Phase-2 API compiles unchanged.

### 2.2 IInputBackend

```cpp
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual void initialize(const InputConfig&) = 0;
    virtual void poll() = 0;                          // one pump, main thread
    virtual std::span<const RawEvent> drain() = 0;    // frame's events
    virtual const RawSnapshot& snapshot() const = 0;  // fused steady state
    virtual void rumble(DeviceId, const HapticEvent&) = 0;
    virtual void set_led(DeviceId, u8 r, u8 g, u8 b) {}
};
```

Three backends:

1. **`NullInputBackend`** — no-op, used on headless CI or tooling.
2. **`TraceReplayBackend`** — drives events from a `.input_trace` file; used in deterministic tests and to reproduce player reports.
3. **`Sdl3InputBackend`** — production. Lives entirely in `engine/platform/input/sdl3_backend.cpp`.

### 2.3 Device identity

`DeviceId` is a 64-bit value:

```
u32 guid_hash    — xxHash32 of SDL's joystick GUID (stable across runs for the same physical device)
u16 player_index — 0-based, assigned in arrival order
u16 flags        — { is_adaptive, is_steam_controller, is_virtual, ... }
```

Hotplug of the same physical device (unplug/replug) keeps `guid_hash`; only the `player_index` may reshuffle. The action-map layer (ADR-0021) binds by `guid_hash`.

### 2.4 Capability mask

```cpp
enum class DeviceCap : u32 {
    Buttons          = 1 << 0,
    Axes             = 1 << 1,
    DPad             = 1 << 2,
    Rumble           = 1 << 3,
    TriggerRumble    = 1 << 4,
    Gyro             = 1 << 5,
    Accel            = 1 << 6,
    Touchpad         = 1 << 7,
    LED              = 1 << 8,
    Adaptive         = 1 << 9,  // Xbox Adaptive Controller auto-flag
    Quadstick        = 1 << 10,
};
```

Deadzone (radial + per-axis) and response curves (linear, quadratic, cubic, custom LUT) are applied **in the backend** — consumers never see raw hardware values.

### 2.5 Hotplug events

SDL emits `SDL_EVENT_GAMEPAD_ADDED/REMOVED/REMAPPED`. The SDL3 backend marshals these into the main-thread event bus as `DeviceConnectedEvent` / `DeviceRemovedEvent`. This is a **cert requirement** for Xbox and Steam Deck and is therefore on the Phase-10 Definition-of-Done checklist.

### 2.6 Haptics

Unified API:

```cpp
struct HapticEvent {
    float low_freq      = 0.0f;  // [0, 1]
    float high_freq     = 0.0f;
    float left_trigger  = 0.0f;  // DualSense / Xbox Series
    float right_trigger = 0.0f;
    u32   duration_ms   = 100;
};
```

`Haptics::play(DeviceId, const HapticEvent&)` queries device capabilities and skips channels the device doesn't support. Global volume scalar + per-device toggle via `haptics.toml`.

### 2.7 Background input policy

Default `SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS=0`. Editor (which has focus juggling) sets it to `1` at init.

### 2.8 Steam init ordering

When Steam is present (`GW_STEAM=1`), `SteamAPI_InitEx` runs before `SDL_Init` so SDL's Steam Input integration sees Steam. The runtime checks `getenv("SteamAppId")` at startup.

### 2.9 `.input_trace` format

Binary little-endian:

```
magic "INTR"  u32  version=1
u64 start_ns
repeated:
  u64 ns_since_start
  u16 kind (device_added, device_removed, button_down, button_up, axis, gyro, ...)
  u16 payload_len
  bytes payload
```

`TraceReplayBackend` consumes one of these and emits deterministic events at replay rate. Recording is enabled via `InputConfig::record_trace_path`.

## 3. Non-goals

- No IME text-input UI (Phase 11).
- No virtual on-screen keyboard (Phase 16 a11y).
- No remote / network input (Phase 14).

## 4. Verification

- `input_trace_roundtrip_test.cpp` — record a synthetic session, replay it, snapshots byte-identical.
- Hotplug fuzz: connect/disconnect every 50 ms for 1 000 iterations, no leaks, no stale handles.
- Deadzone / curve unit tests on known inputs.
- `input_poll_gate` perf test: 4 gamepads connected, poll < 0.1 ms / frame.

## 5. Consequences

Positive:
- CI runs without a physical gamepad.
- Adaptive Controller / Quadstick auto-tagged for the accessibility layer.
- Swapping SDL3 for a future backend is a file-level change.

Negative:
- Two codepaths to maintain (null/trace vs SDL). The trace path is authoritative for determinism tests, which keeps it honest.

## 6. Rollback

Forcing `InputConfig::backend = Null` disables all device input (keyboard/mouse still available through the Phase-2 platform stub). No file changes required.
