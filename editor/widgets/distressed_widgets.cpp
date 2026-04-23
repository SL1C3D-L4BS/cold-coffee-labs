// editor/widgets/distressed_widgets.cpp — Part B §12.3 scaffold.

#include "editor/widgets/distressed_widgets.hpp"

namespace gw::editor::widgets {

// Scaffold impls — real ImGui bindings land in Phase 21 alongside the
// Corrupted Relic aesthetic pass. All functions currently no-op / return
// false so the public API is stable for panel code.

bool DistressedButton(const char* /*label*/) noexcept { return false; }
bool DistressedButton(const char* /*label*/, float /*w*/, float /*h*/) noexcept { return false; }
bool DistressedCheckbox(const char* /*label*/, bool* /*v*/) noexcept { return false; }
bool DistressedSliderFloat(const char* /*label*/, float* /*v*/,
                           float /*min*/, float /*max*/) noexcept { return false; }
void GlitchText(const char* /*label*/) noexcept {}
void BlasphemousProgressBar(float /*fraction*/, std::uint32_t /*color*/) noexcept {}
bool AssetPreviewTile(const char* /*id*/, const char* /*name*/,
                      void* /*tex*/, float /*size*/) noexcept { return false; }

} // namespace gw::editor::widgets
