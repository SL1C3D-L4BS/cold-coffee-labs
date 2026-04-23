#pragma once

#include "engine/narrative/act_state.hpp"

namespace gw::gameplay::acts {

/// Thin gameplay-side component wrapping the narrative Act state. Lives in ECS
/// (one instance per save / session).
struct ActStateComponent {
    narrative::ActState state{};
};

} // namespace gw::gameplay::acts
