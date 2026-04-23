// gameplay/a11y/gameplay_a11y.cpp — Part C §24 scaffold.

#include "gameplay/a11y/gameplay_a11y.hpp"

namespace gw::gameplay::a11y {

void initialize_defaults(GameplayA11yConfig& cfg) noexcept {
    cfg.color_mode = ColorBlindMode::None;
    cfg.gore = GoreLevel::Unleashed;
    cfg.photo = PhotosensitivityProfile{};
    cfg.subtitles = GameplaySubtitles{};
    cfg.audio = AudioSliders{};
    cfg.remap = InputRemapState{};
    cfg.high_contrast_hud = false;
    cfg.reduce_camera_shake = false;
    cfg.one_handed_controls = false;
}

bool maybe_show_photosensitivity_warning(GameplayA11yConfig& cfg) noexcept {
    if (!cfg.photo.warning_shown) {
        // TODO(#gw-a11y-24): push modal via editor/UI layer; persist ack in
        // user settings blob (signed, ADR-0114).
        cfg.photo.warning_shown = true;
    }
    return cfg.photo.warning_shown;
}

void apply(const GameplayA11yConfig& /*cfg*/) noexcept {
    // TODO(#gw-a11y-24): push to engine/a11y CVars, gore LOD table, audio
    // bus mixer, input map snapshot. No-op until gameplay plumbing lands.
}

} // namespace gw::gameplay::a11y
