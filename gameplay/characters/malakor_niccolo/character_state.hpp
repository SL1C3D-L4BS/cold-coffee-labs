#pragma once

#include "engine/narrative/narrative_types.hpp"

#include <cstdint>

namespace gw::gameplay::characters::malakor_niccolo {

/// Dual-persona ECS component. The player never sees Niccoló's face (docs/07
/// §0.2); this holds only the audio-side persona selection state.
struct CharacterState {
    narrative::Speaker last_speaker        = narrative::Speaker::None;
    float              last_line_age_s     = 0.f;
    std::uint32_t      last_line_id        = 0;
    bool               grace_combined_line_fired = false;
};

} // namespace gw::gameplay::characters::malakor_niccolo
