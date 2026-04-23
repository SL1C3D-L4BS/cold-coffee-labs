#pragma once

#include <cstdint>

namespace gw::narrative {

/// Grace Meter — see docs/07 §2.7 and ADR-0101.
/// ECS-resident counter; deterministic accumulation; seed-stamped.
struct GraceComponent {
    float value = 0.f;
    float max   = 100.f;
};

enum class GraceSource : std::uint16_t {
    UnwrittenSpared       = 1,
    CircleNoRapture       = 2,
    GloryKillsOnlyHostile = 3,
    ParryLogosPhase1      = 4,
    BlasphemyNpcSave      = 5,
};

struct GraceTransaction {
    std::uint32_t actor_id = 0;
    GraceSource   source   = GraceSource::UnwrittenSpared;
    float         delta    = 0.f;
    std::uint64_t seed     = 0;
};

[[nodiscard]] float grace_source_default_delta(GraceSource src) noexcept;

/// Apply a grace transaction deterministically. Duplicate transactions from
/// the same `(actor_id, source, seed)` tuple are rejected by the caller
/// (book-keeping lives in the gameplay layer — docs/07 §2.7).
void apply_grace_transaction(GraceComponent& grace, const GraceTransaction& tx) noexcept;

} // namespace gw::narrative
