// editor/theme/editor_fonts.cpp

#include "editor/theme/editor_fonts.hpp"

#include <imgui.h>

namespace gw::editor::theme {

void load_editor_fonts(ImGuiIO& io, const TypographyPack& typo,
                        bool force_mono_metrics) noexcept {
    ImFontConfig cfg{};
    cfg.OversampleH = 2;
    cfg.OversampleV = 1;
    cfg.PixelSnapH  = true;
    const float body = typo.body_px_size > 0.f ? typo.body_px_size : 15.f;
    const float mono = typo.mono_px_size > 0.f ? typo.mono_px_size : 13.f;
    // ImGui 1.92 ImFontConfig no longer exposes per-axis glyph spacing here;
    // mono mode switches to the typography pack's smaller code-friendly size.
    cfg.SizePixels = force_mono_metrics ? mono : body;
    io.Fonts->Clear();
    io.Fonts->AddFontDefault(&cfg);
    io.FontDefault = io.Fonts->Fonts[0];
}

} // namespace gw::editor::theme
