// gameplay/combat/glory_kill.cpp — Phase 22 W147
//
// Intentionally tiny: the evaluator is a constexpr-ish inline in the header.
// This TU exists so the CMake library pulls the symbol graph and so any
// future stateful glory-kill bookkeeping (per-entity-id dedupe, camera
// lock-on curve, etc.) has a dedicated home.

#include "gameplay/combat/glory_kill.hpp"

namespace gw::gameplay::combat {

// Touch the header symbol so a DCE-aggressive linker (e.g. /OPT:REF) keeps
// the inline helper visible from the gameplay DLL.
[[maybe_unused]] static GloryKillVerdict glory_kill_link_anchor() noexcept {
    GloryKillQuery q{};
    return evaluate_glory_kill(q);
}

} // namespace gw::gameplay::combat
