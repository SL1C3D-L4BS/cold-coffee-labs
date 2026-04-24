#pragma once

#include "gameplay/enemies/enemy_bt.hpp"

namespace gw::gameplay::enemies::abyssal {

inline constexpr ArchetypeId kId = ArchetypeId::Abyssal;

[[nodiscard]] inline gw::gameai::BTDesc behavior_tree() noexcept {
    return make_enemy_combat_tree(kId);
}

} // namespace gw::gameplay::enemies::abyssal
