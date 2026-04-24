#pragma once
// gameplay/enemies/cherubim/cherubim.hpp — Phase 23 W149

#include "gameplay/enemies/enemy_bt.hpp"

namespace gw::gameplay::enemies::cherubim {

inline constexpr ArchetypeId kId = ArchetypeId::Cherubim;

[[nodiscard]] inline gw::gameai::BTDesc behavior_tree() noexcept {
    return make_enemy_combat_tree(kId);
}

} // namespace gw::gameplay::enemies::cherubim
