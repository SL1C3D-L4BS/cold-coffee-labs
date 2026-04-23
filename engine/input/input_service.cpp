// engine/input/input_service.cpp
#include "engine/input/input_service.hpp"

#include "engine/input/steam_input_glue.hpp"

#include <utility>

namespace gw::input {

InputService::InputService() = default;
InputService::~InputService() { shutdown(); }

bool InputService::initialize(const InputConfig& cfg) {
    shutdown();
    switch (cfg.backend) {
        case InputBackendKind::Null:
            backend_ = make_null_input_backend();
            break;
        case InputBackendKind::TraceReplay:
            backend_ = make_trace_replay_backend({});
            break;
        case InputBackendKind::Sdl3:
            backend_ = make_sdl3_input_backend();
            break;
    }
    if (!backend_) return false;
    backend_->initialize(cfg);
    haptics_ = std::make_unique<Haptics>(*backend_);

    // pre-eng-steam-input-glue: best-effort Steam Input startup. Non-fatal if
    // Steamworks isn't present (dev builds, CI). When available, the per-frame
    // update loop calls tick_active_scheme() to drive glyph selection.
    (void)gw::input::steam::startup();

    return true;
}

void InputService::shutdown() {
    if (backend_) {
        backend_->shutdown();
        backend_.reset();
    }
    haptics_.reset();
    scanner_.reset(0.0f);
    stack_ = ContextStack{};

    // pre-eng-steam-input-glue: tear down Steam Input so the next init()
    // starts from a clean slate.
    gw::input::steam::shutdown();
}

bool InputService::load_action_map(const std::string& toml) noexcept {
    ActionMap m;
    if (!load_action_map_toml(toml, m)) return false;
    map_ = std::move(m);
    stack_ = ContextStack{};
    if (auto* s = map_.find_set("gameplay")) stack_.push(s);
    refresh_scanner_candidates();
    return true;
}

std::string InputService::save_action_map() const noexcept {
    return save_action_map_toml(map_);
}

bool InputService::push_context(std::string_view set_name) {
    auto* s = map_.find_set(set_name);
    if (!s) return false;
    stack_.push(s);
    refresh_scanner_candidates();
    return true;
}

void InputService::pop_context() {
    stack_.pop();
    refresh_scanner_candidates();
}

void InputService::update(float now_ms) {
    if (!backend_) return;
    backend_->poll();
    auto events = backend_->drain();
    for (const auto& e : events) route_event(e);
    stats_.events_processed += events.size();
    snapshot_ = backend_->snapshot();
    // Evaluate actions in every set on the context stack; top-of-stack wins
    // on duplicate names (queries resolve top-down).
    for (auto it = stack_.stack().rbegin(); it != stack_.stack().rend(); ++it) {
        if (*it) evaluate_set(**it, snapshot_, now_ms);
    }
    if (scanner_enabled_) scanner_.tick(now_ms);

    // pre-eng-steam-input-glue: sample which control scheme Steam last saw so
    // UI glyph resolution stays in sync with the player's active device.
    (void)gw::input::steam::tick_active_scheme();

    ++stats_.frames_updated;
}

void InputService::route_event(const RawEvent& e) noexcept {
    switch (e.kind) {
        case RawEventKind::DeviceAdded: {
            Device d{};
            d.id = e.device;
            d.kind = DeviceKind::Gamepad;
            d.caps = Cap_Buttons | Cap_Axes;
            if (devices_.on_connected(d)) ++stats_.devices_connected;
            break;
        }
        case RawEventKind::DeviceRemoved:
            devices_.on_removed(e.device);
            break;
        default:
            break;
    }
    // Capture for rebind flow.
    if (rebind_active_) {
        Binding b = binding_from_event(e);
        if (b.source != BindingSource::None) {
            captured_binding_ = b;
        }
    }
}

const Action* InputService::action(std::string_view name) const noexcept {
    const auto& sets = stack_.stack();
    for (auto it = sets.rbegin(); it != sets.rend(); ++it) {
        if (!*it) continue;
        if (const auto* a = (*it)->find(name); a) return a;
    }
    return nullptr;
}

Action* InputService::action(std::string_view name) noexcept {
    const auto& sets = stack_.stack();
    for (auto it = sets.rbegin(); it != sets.rend(); ++it) {
        if (!*it) continue;
        if (auto* a = (*it)->find(name); a) return a;
    }
    return nullptr;
}

void InputService::begin_rebind(std::string_view action_name) noexcept {
    rebind_active_ = true;
    rebind_action_name_ = std::string(action_name);
    captured_binding_ = Binding{};
}

bool InputService::finish_rebind() noexcept {
    if (!rebind_active_) return false;
    if (captured_binding_.source == BindingSource::None) {
        rebind_active_ = false;
        return false;
    }
    if (auto* set = stack_.top()) {
        apply_rebind(*set, rebind_action_name_, captured_binding_);
    }
    rebind_active_ = false;
    return true;
}

void InputService::cancel_rebind() noexcept {
    rebind_active_ = false;
    captured_binding_ = Binding{};
}

void InputService::enable_scanner(bool on) noexcept {
    scanner_enabled_ = on;
    if (on) refresh_scanner_candidates();
}

void InputService::apply_profile(const AdaptiveProfile& p) {
    ::gw::input::apply_profile(map_, p);
    if (p.auto_enable_scanner) enable_scanner(true);
    stack_ = ContextStack{};
    if (auto* s = map_.find_set("gameplay")) stack_.push(s);
    refresh_scanner_candidates();
}

void InputService::refresh_scanner_candidates() {
    scanner_.set_candidates(gather_scan_candidates(stack_));
}

} // namespace gw::input
