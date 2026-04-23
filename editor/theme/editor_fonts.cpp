// editor/theme/editor_fonts.cpp

#include "editor/theme/editor_fonts.hpp"

#include <imgui.h>

namespace gw::editor::theme {

void load_editor_fonts(ImGuiIO& io, const TypographyPack& typo) noexcept {
    ImFontConfig cfg{};
    cfg.OversampleH = 2;
    cfg.OversampleV = 1;
    cfg.PixelSnapH  = true;
    cfg.SizePixels  = typo.body_px_size > 0.f ? typo.body_px_size : 15.f;
    io.Fonts->Clear();
    io.Fonts->AddFontDefault(&cfg);
    io.FontDefault = io.Fonts->Fonts[0];
}

} // namespace gw::editor::theme
