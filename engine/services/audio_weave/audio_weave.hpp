#pragma once

#include "engine/services/audio_weave/schema/music.hpp"

namespace gw::services::audio_weave {

[[nodiscard]] MusicResult mix(const MusicRequest& req) noexcept;

} // namespace gw::services::audio_weave
