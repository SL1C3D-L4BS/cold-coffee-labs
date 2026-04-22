#pragma once

// Public POD ECS components for gameplay_module / native mods (Phase 18-C).
// Kept separate from editor/scene types so mods never depend on editor headers.

namespace gw {
namespace scripting {

/// World-space translation for mod-authored entities (float64 per engine convention).
/// Mods should use this type instead of editor-only transform components.
struct ModTransformComponent {
    double position_x = 0.0;
    double position_y = 0.0;
    double position_z = 0.0;
};

}  // namespace scripting
}  // namespace gw
