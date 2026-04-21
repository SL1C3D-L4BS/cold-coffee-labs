#pragma once
// engine/input/input_backend.hpp — IInputBackend + null + trace backends.
//
// The SDL3 production backend lives under engine/platform/input/ and is
// gated behind GW_INPUT_SDL3. The null and trace-replay backends compile
// everywhere.

#include "engine/input/input_types.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace gw::input {

class IInputBackend {
public:
    virtual ~IInputBackend() = default;

    virtual void initialize(const InputConfig&) = 0;
    virtual void shutdown() noexcept = 0;

    virtual void poll() = 0;
    virtual std::span<const RawEvent> drain() = 0;
    virtual const RawSnapshot& snapshot() const = 0;

    virtual void rumble(DeviceId, const HapticEvent&) {}
    virtual void set_led(DeviceId, uint8_t, uint8_t, uint8_t) {}

    virtual const char* name() const noexcept = 0;

    // Test hook — injects a raw event for the null/trace backends.
    virtual void inject_for_test(const RawEvent&) {}
};

// Factories.
[[nodiscard]] std::unique_ptr<IInputBackend> make_null_input_backend();

// Replay backend — consumes RawEvents from a previously recorded trace.
[[nodiscard]] std::unique_ptr<IInputBackend>
make_trace_replay_backend(std::vector<RawEvent> trace);

// Production SDL3 backend (gated). When GW_INPUT_SDL3 is undefined, this
// falls back to the null backend.
[[nodiscard]] std::unique_ptr<IInputBackend> make_sdl3_input_backend();

} // namespace gw::input
