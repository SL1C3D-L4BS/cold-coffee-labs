#pragma once
// engine/input/input_service.hpp — the public input API.
//
// One RAII façade per app. Owns the backend, device registry, haptics,
// and the active action map / context stack. Worker thread (or main
// thread) pumps events each frame; consumers query `action()` or
// `snapshot()`.

#include "engine/input/action.hpp"
#include "engine/input/action_map_toml.hpp"
#include "engine/input/adaptive_profile.hpp"
#include "engine/input/device.hpp"
#include "engine/input/haptics.hpp"
#include "engine/input/input_backend.hpp"
#include "engine/input/rebind.hpp"
#include "engine/input/scanner.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace gw::input {

class InputService {
public:
    struct Stats {
        uint64_t events_processed{0};
        uint64_t frames_updated{0};
        uint32_t devices_connected{0};
    };

    InputService();
    ~InputService();

    InputService(const InputService&) = delete;
    InputService& operator=(const InputService&) = delete;

    // Initialize the backend. `cfg.replay_trace_path` forces TraceReplay.
    [[nodiscard]] bool initialize(const InputConfig& cfg);
    void shutdown();

    // ---- Map / context ----
    [[nodiscard]] ActionMap& action_map() noexcept { return map_; }
    [[nodiscard]] const ActionMap& action_map() const noexcept { return map_; }
    [[nodiscard]] ContextStack& context_stack() noexcept { return stack_; }
    [[nodiscard]] const ContextStack& context_stack() const noexcept { return stack_; }

    bool load_action_map(const std::string& toml_source) noexcept;
    [[nodiscard]] std::string save_action_map() const noexcept;

    // Push a set by name onto the context stack (returns false if missing).
    bool push_context(std::string_view set_name);
    void pop_context();

    // ---- Frame tick ----
    // Pull events from the backend, advance the raw snapshot, evaluate
    // actions, run scanner ticks. `now_ms` is monotonic game time.
    void update(float now_ms);

    // ---- Queries ----
    [[nodiscard]] const RawSnapshot& snapshot() const noexcept { return snapshot_; }
    // Look up an action by name, starting from the top of the context
    // stack. Returns nullptr if no active set defines it.
    [[nodiscard]] const Action* action(std::string_view name) const noexcept;
    [[nodiscard]] Action*       action(std::string_view name) noexcept;
    [[nodiscard]] Haptics&      haptics() noexcept { return *haptics_; }
    [[nodiscard]] DeviceRegistry& devices() noexcept { return devices_; }
    [[nodiscard]] const Stats&  stats() const noexcept { return stats_; }

    // ---- Rebind ----
    // Start / finish a rebind. While rebinding, the next eligible event is
    // captured and offered through `finish_rebind`. Callers must route
    // raw events via `feed_event_for_rebind` during capture.
    void begin_rebind(std::string_view action_name) noexcept;
    bool finish_rebind() noexcept;   // installs captured binding; returns true if installed
    void cancel_rebind() noexcept;
    [[nodiscard]] bool is_rebinding() const noexcept { return rebind_active_; }
    [[nodiscard]] const Binding& captured_binding() const noexcept { return captured_binding_; }

    // ---- Scanner ----
    [[nodiscard]] SingleSwitchScanner& scanner() noexcept { return scanner_; }
    void enable_scanner(bool on) noexcept;

    // ---- Accessibility: push a profile overlay (adaptive / quadstick). ----
    void apply_profile(const AdaptiveProfile& p);

private:
    void route_event(const RawEvent& e) noexcept;
    void refresh_scanner_candidates();

    std::unique_ptr<IInputBackend>  backend_{};
    DeviceRegistry                   devices_{};
    std::unique_ptr<Haptics>         haptics_{};
    ActionMap                        map_{};
    ContextStack                     stack_{};
    RawSnapshot                      snapshot_{};
    SingleSwitchScanner              scanner_{};
    Stats                            stats_{};

    bool     rebind_active_{false};
    std::string rebind_action_name_{};
    Binding  captured_binding_{};
    bool     scanner_enabled_{false};
};

} // namespace gw::input
