// editor/theme/distressed_draw.cpp — Part B §12.1 scaffold.
//
// Scaffold only; real ImDrawList wiring lands in Phase 21 with the
// Corrupted Relic aesthetic. These no-op stubs keep the public API stable
// so panel code can compile against it today.

#include "editor/theme/distressed_draw.hpp"

namespace gw::editor::theme {

void draw_jagged_border(float, float, float, float, const DistressedParams&) noexcept {}
void draw_pixel_jitter_hover(float, float, float, float, std::uint32_t,
                             const DistressedParams&) noexcept {}
void draw_crack_separator(float, float, float, float, const DistressedParams&) noexcept {}

} // namespace gw::editor::theme
