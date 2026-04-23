// gameplay/boss/logos/phase4_selector.cpp — Phase 23 scaffold.

#include "gameplay/boss/logos/phase4_selector.hpp"

namespace gw::gameplay::boss::logos {

Phase4Branch select_phase4_branch(const narrative::GraceComponent& grace) noexcept {
    return grace.value >= grace.max ? Phase4Branch::Grace
                                    : Phase4Branch::Annihilation;
}

} // namespace gw::gameplay::boss::logos
