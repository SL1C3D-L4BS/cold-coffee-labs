#pragma once
// Phase 23 W150

#include "gameplay/enemies/enemy_bt.hpp"

namespace gw::gameplay::enemies::martyr {

inline constexpr ArchetypeId kId = ArchetypeId::Martyr;

[[nodiscard]] inline gw::gameai::BTDesc behavior_tree() noexcept {
    return make_enemy_combat_tree(kId);
}

} // namespace gw::gameplay::enemies::martyr
