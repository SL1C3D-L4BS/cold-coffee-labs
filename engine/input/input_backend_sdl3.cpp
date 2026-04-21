// engine/input/input_backend_sdl3.cpp
//
// Production SDL3 backend (ADR-0020). Gated behind GW_INPUT_SDL3. When the
// flag is off, the factory returns a null backend so headless and CI
// builds link without SDL3.
#include "engine/input/input_backend.hpp"

#if defined(GW_INPUT_SDL3) && GW_INPUT_SDL3
  // Real SDL3 implementation lives in engine/platform/input/sdl3_input.cpp
  // (to keep SDL headers quarantined). Declared here:
  namespace gw::input::platform {
    [[nodiscard]] std::unique_ptr<IInputBackend> make_sdl3_backend_impl();
  }
  namespace gw::input {
    std::unique_ptr<IInputBackend> make_sdl3_input_backend() {
        return platform::make_sdl3_backend_impl();
    }
  }
#else
  namespace gw::input {
    std::unique_ptr<IInputBackend> make_sdl3_input_backend() {
        // No SDL3 available this build — fall back to null.
        return make_null_input_backend();
    }
  }
#endif
