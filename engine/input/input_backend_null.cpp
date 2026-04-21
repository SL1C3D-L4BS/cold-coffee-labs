// engine/input/input_backend_null.cpp — null + trace-replay implementations.
#include "engine/input/input_backend.hpp"

#include <algorithm>

namespace gw::input {

namespace {

void apply_event_to_snapshot(const RawEvent& ev, RawSnapshot& s) {
    switch (ev.kind) {
        case RawEventKind::KeyDown: {
            const auto k = ev.code;
            if (k < 256) s.keys[k / 64] |= (1ULL << (k % 64));
            s.modifiers = ev.modifiers;
            break;
        }
        case RawEventKind::KeyUp: {
            const auto k = ev.code;
            if (k < 256) s.keys[k / 64] &= ~(1ULL << (k % 64));
            s.modifiers = ev.modifiers;
            break;
        }
        case RawEventKind::MouseMove:
            s.mouse_dx = ev.fval[0];
            s.mouse_dy = ev.fval[1];
            s.mouse_x += ev.fval[0];
            s.mouse_y += ev.fval[1];
            break;
        case RawEventKind::MouseWheel:
            s.mouse_wheel = ev.fval[1];
            break;
        case RawEventKind::MouseButtonDown:
            s.mouse_buttons |= (1u << ev.code);
            break;
        case RawEventKind::MouseButtonUp:
            s.mouse_buttons &= ~(1u << ev.code);
            break;
        case RawEventKind::DeviceAdded: {
            for (auto& g : s.gamepads) {
                if (!g.present) { g.present = true; g.id = ev.device; break; }
            }
            break;
        }
        case RawEventKind::DeviceRemoved:
            for (auto& g : s.gamepads) {
                if (g.present && g.id == ev.device) { g.present = false; g.id = {}; break; }
            }
            break;
        case RawEventKind::GamepadButtonDown: {
            for (auto& g : s.gamepads) {
                if (g.present && g.id == ev.device) { g.buttons |= (1u << ev.code); break; }
            }
            break;
        }
        case RawEventKind::GamepadButtonUp: {
            for (auto& g : s.gamepads) {
                if (g.present && g.id == ev.device) { g.buttons &= ~(1u << ev.code); break; }
            }
            break;
        }
        case RawEventKind::GamepadAxis_: {
            for (auto& g : s.gamepads) {
                if (g.present && g.id == ev.device) {
                    const auto ai = ev.code;
                    if (ai < static_cast<uint32_t>(GamepadAxis::Count)) {
                        g.axes[ai] = ev.fval[0];
                    }
                    break;
                }
            }
            break;
        }
        case RawEventKind::GamepadGyro: {
            for (auto& g : s.gamepads) {
                if (g.present && g.id == ev.device) {
                    g.gyro = math::Vec3f(ev.fval[0], ev.fval[1], ev.fval[2]);
                    break;
                }
            }
            break;
        }
        case RawEventKind::GamepadAccel: {
            for (auto& g : s.gamepads) {
                if (g.present && g.id == ev.device) {
                    g.accel = math::Vec3f(ev.fval[0], ev.fval[1], ev.fval[2]);
                    break;
                }
            }
            break;
        }
        default: break;
    }
}

class NullInputBackend final : public IInputBackend {
public:
    void initialize(const InputConfig&) override {}
    void shutdown() noexcept override {}

    void poll() override {
        pending_drained_ = std::move(pending_);
        pending_.clear();
    }

    std::span<const RawEvent> drain() override {
        return std::span<const RawEvent>(pending_drained_.data(), pending_drained_.size());
    }

    const RawSnapshot& snapshot() const override { return snapshot_; }

    void inject_for_test(const RawEvent& ev) override {
        pending_.push_back(ev);
        apply_event_to_snapshot(ev, snapshot_);
    }

    const char* name() const noexcept override { return "null"; }

private:
    std::vector<RawEvent> pending_;
    std::vector<RawEvent> pending_drained_;
    RawSnapshot           snapshot_;
};

class TraceReplayBackend final : public IInputBackend {
public:
    explicit TraceReplayBackend(std::vector<RawEvent> trace) : trace_(std::move(trace)) {}

    void initialize(const InputConfig&) override {}
    void shutdown() noexcept override {}

    void poll() override {
        // Drain one "frame" of trace events. In the simplest replay, each
        // poll advances the cursor to the next group with the same (quantised)
        // timestamp. We use a coarse 16 ms grouping.
        drained_.clear();
        if (cursor_ >= trace_.size()) return;
        const uint64_t group_ns = trace_[cursor_].timestamp_ns / (16'000'000ULL);
        while (cursor_ < trace_.size() &&
               trace_[cursor_].timestamp_ns / (16'000'000ULL) == group_ns) {
            apply_event_to_snapshot(trace_[cursor_], snapshot_);
            drained_.push_back(trace_[cursor_]);
            ++cursor_;
        }
    }

    std::span<const RawEvent> drain() override {
        return std::span<const RawEvent>(drained_.data(), drained_.size());
    }

    const RawSnapshot& snapshot() const override { return snapshot_; }

    const char* name() const noexcept override { return "trace"; }

    [[nodiscard]] bool exhausted() const noexcept { return cursor_ >= trace_.size(); }

private:
    std::vector<RawEvent> trace_;
    std::vector<RawEvent> drained_;
    std::size_t           cursor_{0};
    RawSnapshot           snapshot_;
};

} // namespace

std::unique_ptr<IInputBackend> make_null_input_backend() {
    return std::make_unique<NullInputBackend>();
}

std::unique_ptr<IInputBackend>
make_trace_replay_backend(std::vector<RawEvent> trace) {
    return std::make_unique<TraceReplayBackend>(std::move(trace));
}

} // namespace gw::input
