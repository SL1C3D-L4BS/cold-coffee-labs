#pragma once
// engine/telemetry/consent_fsm.hpp — ADR-0062 consent state machine.
//
// States:
//   Initial → LegalNotice → AgeGate → ConsentDisaggregated → Persisted
//   Persisted → (Revocable) → ConsentDisaggregated (back-edge)
//
// The FSM is headless by design: the RmlUi templates under `ui/privacy/`
// drive it through `advance()` calls. Tests exercise every legal transition
// with no runtime dependency on RmlUi.

#include "engine/telemetry/consent.hpp"

#include <cstdint>
#include <string_view>

namespace gw::telemetry {

enum class ConsentFsmState : std::uint8_t {
    Initial = 0,
    LegalNotice,
    AgeGate,
    ConsentDisaggregated,
    Persisted,
    Revocable,
};

[[nodiscard]] std::string_view to_string(ConsentFsmState s) noexcept;

struct ConsentFsmInputs {
    bool        legal_acknowledged{false};
    int         age_years{-1};          // -1 = declined to answer (zero-data path).
    int         gate_years{13};
    ConsentTier chosen_tier{ConsentTier::None};
    bool        confirm_save{false};    // user pressed "Continue" on persisted screen.
    bool        revoke_requested{false};
};

struct ConsentFsmSnapshot {
    ConsentFsmState state{ConsentFsmState::Initial};
    ConsentTier     effective_tier{ConsentTier::None};
    int             effective_age{-1};
};

/// Pure FSM — `advance(current, inputs)` returns the next snapshot.
[[nodiscard]] ConsentFsmSnapshot consent_fsm_advance(ConsentFsmSnapshot current,
                                                     const ConsentFsmInputs& inputs) noexcept;

} // namespace gw::telemetry
