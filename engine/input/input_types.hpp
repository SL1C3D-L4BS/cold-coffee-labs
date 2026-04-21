#pragma once
// engine/input/input_types.hpp — Phase-10 input primitives (ADR-0020).
//
// Downstream code (ECS systems, editor UI, game logic) sees only these
// types. OS-level APIs (SDL3, evdev, XInput) live under
// engine/platform/input/ and never escape their translation unit.

#include "engine/math/vec.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace gw::input {

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::int16_t;
using std::int32_t;

// ---------------------------------------------------------------------------
// Device identity
// ---------------------------------------------------------------------------

struct DeviceId {
    uint64_t value{0};

    [[nodiscard]] constexpr bool is_null() const noexcept { return value == 0; }
    [[nodiscard]] constexpr uint32_t guid_hash() const noexcept {
        return static_cast<uint32_t>(value & 0xFFFFFFFFu);
    }
    [[nodiscard]] constexpr uint16_t player_index() const noexcept {
        return static_cast<uint16_t>((value >> 32) & 0xFFFFu);
    }
    [[nodiscard]] constexpr uint16_t flags() const noexcept {
        return static_cast<uint16_t>((value >> 48) & 0xFFFFu);
    }
    [[nodiscard]] constexpr bool operator==(const DeviceId&) const noexcept = default;

    [[nodiscard]] static constexpr DeviceId make(uint32_t guid, uint16_t player, uint16_t flags) noexcept {
        return DeviceId{static_cast<uint64_t>(guid)
                        | (static_cast<uint64_t>(player) << 32)
                        | (static_cast<uint64_t>(flags) << 48)};
    }
};

// ---------------------------------------------------------------------------
// Capability mask
// ---------------------------------------------------------------------------

enum DeviceCap : uint32_t {
    Cap_None          = 0,
    Cap_Buttons       = 1u << 0,
    Cap_Axes          = 1u << 1,
    Cap_DPad          = 1u << 2,
    Cap_Rumble        = 1u << 3,
    Cap_TriggerRumble = 1u << 4,
    Cap_Gyro          = 1u << 5,
    Cap_Accel         = 1u << 6,
    Cap_Touchpad      = 1u << 7,
    Cap_LED           = 1u << 8,
    Cap_Adaptive      = 1u << 9,  // Xbox Adaptive Controller
    Cap_Quadstick     = 1u << 10,
};

[[nodiscard]] inline constexpr bool has_cap(uint32_t caps, DeviceCap c) noexcept {
    return (caps & static_cast<uint32_t>(c)) != 0;
}

// ---------------------------------------------------------------------------
// Device kind
// ---------------------------------------------------------------------------

enum class DeviceKind : uint8_t {
    Unknown = 0,
    Keyboard,
    Mouse,
    Gamepad,
    HID,       // generic HID (flight stick, adaptive controller, quadstick)
};

// ---------------------------------------------------------------------------
// Input codes (stable, cross-platform)
// ---------------------------------------------------------------------------

// Subset of keyboard keys used by default bindings. Full table grows over
// time; additions must preserve numeric values (used in .input_trace).
enum class KeyCode : uint16_t {
    None = 0,
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Key1 = 30, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9, Key0,
    Return = 40, Escape, Backspace, Tab, Space,
    Minus, Equals, LeftBracket, RightBracket, Backslash,
    Semicolon = 51, Apostrophe, Grave, Comma, Period, Slash,
    CapsLock = 57,
    F1 = 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Right = 79, Left, Down, Up,
    LCtrl = 224, LShift, LAlt, LMeta, RCtrl, RShift, RAlt, RMeta,
};

enum class MouseButton : uint16_t {
    Left = 1, Right, Middle, X1, X2,
};

enum class GamepadButton : uint16_t {
    A = 0, B, X, Y,
    Back, Guide, Start,
    LeftStick, RightStick,
    LeftShoulder, RightShoulder,
    DPadUp, DPadDown, DPadLeft, DPadRight,
    Misc1, Paddle1, Paddle2, Paddle3, Paddle4, Touchpad,
    Count
};

enum class GamepadAxis : uint16_t {
    LeftX = 0, LeftY,
    RightX, RightY,
    TriggerLeft, TriggerRight,
    Count
};

enum class ModifierBit : uint16_t {
    None      = 0,
    Shift     = 1 << 0,
    Ctrl      = 1 << 1,
    Alt       = 1 << 2,
    Meta      = 1 << 3,
    Caps      = 1 << 4,
};
using ModifierMask = uint16_t;

// ---------------------------------------------------------------------------
// Raw events (backend → input service)
// ---------------------------------------------------------------------------

enum class RawEventKind : uint16_t {
    Noop = 0,
    DeviceAdded,
    DeviceRemoved,
    KeyDown,
    KeyUp,
    MouseMove,
    MouseWheel,
    MouseButtonDown,
    MouseButtonUp,
    GamepadButtonDown,
    GamepadButtonUp,
    GamepadAxis_,
    GamepadGyro,
    GamepadAccel,
    TextInput,
};

struct RawEvent {
    RawEventKind   kind{RawEventKind::Noop};
    DeviceId       device{};
    uint64_t       timestamp_ns{0};
    uint32_t       code{0};     // keycode, button, axis index, etc.
    float          fval[3]{0, 0, 0};   // axis/sensor payload (x, y, z)
    int16_t        ival[2]{0, 0};      // mouse delta
    ModifierMask   modifiers{0};
};

// ---------------------------------------------------------------------------
// Steady-state snapshot (service → consumers)
// ---------------------------------------------------------------------------

struct RawSnapshot {
    // Keyboard: bitset indexed by KeyCode; we use an array of uint64_t for
    // the first 256 keys, covering USB HID keyboard page.
    std::array<uint64_t, 4> keys{};
    // Mouse:
    float mouse_x{0.0f}, mouse_y{0.0f};
    float mouse_dx{0.0f}, mouse_dy{0.0f};
    float mouse_wheel{0.0f};
    uint32_t mouse_buttons{0};   // bit per button
    ModifierMask modifiers{0};

    // Gamepads: up to 4 fused.
    struct GamepadState {
        DeviceId     id{};
        uint32_t     buttons{0};
        float        axes[static_cast<std::size_t>(GamepadAxis::Count)]{};
        math::Vec3f  gyro{0, 0, 0};
        math::Vec3f  accel{0, 0, 0};
        bool         present{false};
    };
    std::array<GamepadState, 4> gamepads{};

    [[nodiscard]] bool key_down(KeyCode k) const noexcept {
        const auto kv = static_cast<uint32_t>(k);
        if (kv >= 256) return false;
        return (keys[kv / 64] & (1ULL << (kv % 64))) != 0;
    }
};

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

enum class InputBackendKind : uint8_t {
    Null = 0,
    TraceReplay,
    Sdl3,
};

struct InputConfig {
    InputBackendKind backend{InputBackendKind::Null};
    bool             background_events{false};
    const char*      replay_trace_path{nullptr};
    const char*      record_trace_path{nullptr};
};

// ---------------------------------------------------------------------------
// Haptics
// ---------------------------------------------------------------------------

struct HapticEvent {
    float    low_freq{0.0f};
    float    high_freq{0.0f};
    float    left_trigger{0.0f};
    float    right_trigger{0.0f};
    uint32_t duration_ms{100};
};

} // namespace gw::input
