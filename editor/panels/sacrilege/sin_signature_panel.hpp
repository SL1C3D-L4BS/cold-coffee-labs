#pragma once
// editor/panels/sacrilege/sin_signature_panel.hpp — Phase 22 W148.

#include "editor/panels/panel.hpp"
#include "engine/narrative/sin_signature.hpp"

#include <cstdint>

namespace gw::editor::panels::sacrilege {

/// Live Sin-signature readout + authorable reaction thresholds. The runtime
/// VoiceDirector consumes these thresholds to route dialogue.
struct SinSignatureReactionThresholds {
    float cruelty_to_malakor = 0.6f;   // ≥ → Malakor bias
    float precision_to_niccolo = 0.6f; // ≥ → Niccolo bias
    float god_mode_uptime_warning = 0.4f;
    float deaths_per_area_warning = 2.0f;
};

class SinSignaturePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Sin-Signature"; }

    void                  set_live_signature(const gw::narrative::SinSignature& sig) noexcept { live_ = sig; }
    [[nodiscard]] const gw::narrative::SinSignature& live_signature() const noexcept { return live_; }
    SinSignatureReactionThresholds&       thresholds()       noexcept { return thresholds_; }
    [[nodiscard]] const SinSignatureReactionThresholds& thresholds() const noexcept { return thresholds_; }

private:
    gw::narrative::SinSignature       live_{};
    SinSignatureReactionThresholds    thresholds_{};
};

} // namespace gw::editor::panels::sacrilege
