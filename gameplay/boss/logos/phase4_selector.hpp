#pragma once

#include "engine/narrative/grace_meter.hpp"
#include "engine/narrative/sin_signature.hpp"

namespace gw::gameplay::boss::logos {

enum class Phase4Branch {
    Annihilation = 0,
    Grace        = 1,
};

struct Phase4Selection {
    Phase4Branch  branch                 = Phase4Branch::Annihilation;
    /// Cooked VO table index — derived from Sin-signature (player-voice timbre lane).
    std::uint32_t logos_voice_lane       = 0;
};

/// Deterministic selector: picks the Phase-4 branch at the entry window and
/// commits. No rollback across the selection (docs/07 §7b).
[[nodiscard]] Phase4Branch select_phase4_branch(const narrative::GraceComponent& grace) noexcept;

[[nodiscard]] Phase4Selection select_phase4(const narrative::GraceComponent& grace,
                                            const narrative::SinSignature&   sig) noexcept;

} // namespace gw::gameplay::boss::logos
