#pragma once

#include "engine/narrative/grace_meter.hpp"

namespace gw::gameplay::boss::logos {

enum class Phase4Branch {
    Annihilation = 0,
    Grace        = 1,
};

/// Deterministic selector: picks the Phase-4 branch at the entry window and
/// commits. No rollback across the selection (docs/07 §7b).
[[nodiscard]] Phase4Branch select_phase4_branch(const narrative::GraceComponent& grace) noexcept;

} // namespace gw::gameplay::boss::logos
