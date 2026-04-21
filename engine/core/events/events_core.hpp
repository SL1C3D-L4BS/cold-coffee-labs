#pragma once
// engine/core/events/events_core.hpp — the frozen Phase-11 engine event
// catalogue (ADR-0023). Adding a new event here is an ADR-worthy change.

#include <cstdint>
#include <string_view>

namespace gw::events {

// Kind IDs for CrossSubsystemBus. Numeric values are the C-ABI contract;
// append only, never reorder.
enum class EventKind : std::uint32_t {
    DeviceHotplug          = 0x0001,
    ActionTriggered        = 0x0002,
    WindowResized          = 0x0003,
    FocusChanged           = 0x0004,
    AudioMarkerCrossed     = 0x0005,
    ConfigCVarChanged      = 0x0006,
    ConsoleCommandExecuted = 0x0007,
    UILayoutComplete       = 0x0008,
    // Reserved 0x0009..0x0FFF for engine-core events.
    // 0x1000..0x1FFF: BLD → engine.
    // 0x2000..0x2FFF: engine → BLD.
};

// ---------------------------------------------------------------------------
// DeviceHotplug — Phase-10 InputService emits this on attach/detach.
// ---------------------------------------------------------------------------
struct DeviceHotplug {
    std::uint64_t device_id{0};
    std::uint16_t player_index{0};
    bool          attached{false};
};

// ---------------------------------------------------------------------------
// ActionTriggered — in-frame deferred, drained by HUD + audio cue system.
// ---------------------------------------------------------------------------
struct ActionTriggered {
    std::uint32_t action_name_hash{0};   // fnv1a32 of the action name
    float         value_f{0.0f};
    std::uint64_t timestamp_ns{0};
};

// ---------------------------------------------------------------------------
// WindowResized — UIService + render swapchain subscribe.
// ---------------------------------------------------------------------------
struct WindowResized {
    std::uint32_t width{0};
    std::uint32_t height{0};
    float         dpi_scale{1.0f};
};

// ---------------------------------------------------------------------------
// FocusChanged — UIService + InputService (release-all-keys) subscribe.
// ---------------------------------------------------------------------------
struct FocusChanged {
    bool          focused{false};
    std::uint64_t window_id{0};
};

// ---------------------------------------------------------------------------
// AudioMarkerCrossed — music-streamer callbacks, drained into caption
// channel for subtitles.
// ---------------------------------------------------------------------------
struct AudioMarkerCrossed {
    std::uint32_t marker_name_hash{0};
    std::uint32_t speaker_id{0};
    std::uint64_t clip_id{0};
    std::uint64_t timestamp_ns{0};
};

// ---------------------------------------------------------------------------
// ConfigCVarChanged — every subsystem may subscribe.
// `origin` is the CVarOrigin enum from engine/core/config; see ADR-0024.
// ---------------------------------------------------------------------------
struct ConfigCVarChanged {
    std::uint32_t cvar_id{0};
    std::uint32_t origin{0};  // bit 0: programmatic, 1: console, 2: toml,
                              //        3: rml-binding, 4: bld
};

// ---------------------------------------------------------------------------
// ConsoleCommandExecuted — audit log, BLD replay.
// ---------------------------------------------------------------------------
struct ConsoleCommandExecuted {
    std::uint32_t command_name_hash{0};
    std::int32_t  exit_code{0};
    std::uint32_t argv_chars{0};
};

// ---------------------------------------------------------------------------
// UILayoutComplete — in-frame deferred; drained by render pass ordering.
// ---------------------------------------------------------------------------
struct UILayoutComplete {
    std::uint64_t frame_index{0};
    std::uint32_t elements_rendered{0};
};

// ---------------------------------------------------------------------------
// fnv1a32 — stable hash used throughout events_core (and ConsoleService).
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr std::uint32_t fnv1a32(std::string_view s) noexcept {
    std::uint32_t h = 0x811C9DC5u;
    for (char c : s) {
        h ^= static_cast<std::uint32_t>(static_cast<unsigned char>(c));
        h *= 0x01000193u;
    }
    return h;
}

static_assert(fnv1a32("") == 0x811C9DC5u);

} // namespace gw::events
