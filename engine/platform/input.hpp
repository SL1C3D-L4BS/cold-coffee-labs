#pragma once

#include <cstdint>

namespace gw::platform {

enum class KeyState : std::uint8_t {
    Released = 0,
    Pressed = 1,
};

struct MouseState {
    double x{0.0};
    double y{0.0};
    KeyState left{KeyState::Released};
    KeyState right{KeyState::Released};
    KeyState middle{KeyState::Released};
};

}  // namespace gw::platform
