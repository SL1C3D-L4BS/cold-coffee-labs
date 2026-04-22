// engine/telemetry/consent_fsm.cpp — ADR-0062 consent FSM.

#include "engine/telemetry/consent_fsm.hpp"

namespace gw::telemetry {

std::string_view to_string(ConsentFsmState s) noexcept {
    switch (s) {
    case ConsentFsmState::Initial:              return "Initial";
    case ConsentFsmState::LegalNotice:          return "LegalNotice";
    case ConsentFsmState::AgeGate:              return "AgeGate";
    case ConsentFsmState::ConsentDisaggregated: return "ConsentDisaggregated";
    case ConsentFsmState::Persisted:            return "Persisted";
    case ConsentFsmState::Revocable:            return "Revocable";
    }
    return "?";
}

ConsentFsmSnapshot consent_fsm_advance(ConsentFsmSnapshot current,
                                      const ConsentFsmInputs& inputs) noexcept {
    ConsentFsmSnapshot next = current;
    switch (current.state) {
    case ConsentFsmState::Initial:
        next.state = ConsentFsmState::LegalNotice;
        break;
    case ConsentFsmState::LegalNotice:
        if (inputs.legal_acknowledged) next.state = ConsentFsmState::AgeGate;
        break;
    case ConsentFsmState::AgeGate:
        next.effective_age = inputs.age_years;
        next.state         = ConsentFsmState::ConsentDisaggregated;
        break;
    case ConsentFsmState::ConsentDisaggregated:
        next.effective_tier =
            apply_age_gate(inputs.chosen_tier, next.effective_age, inputs.gate_years);
        if (inputs.confirm_save) next.state = ConsentFsmState::Persisted;
        break;
    case ConsentFsmState::Persisted:
        if (inputs.revoke_requested) next.state = ConsentFsmState::Revocable;
        break;
    case ConsentFsmState::Revocable:
        next.state = ConsentFsmState::ConsentDisaggregated;
        break;
    }
    return next;
}

} // namespace gw::telemetry
