// editor/main.cpp
// Greywater Editor — Phase 7: Editor Foundation
// Spec ref: docs/05 Phase 7 (weeks 037-042), Phase 7 specification §2.
//
// Entry point: constructs EditorApplication, which owns the GLFW window,
// Vulkan device, ImGui docking host, and all panels.
#include "app/editor_app.hpp"
#include "engine/core/version.hpp"
#include "engine/core/log.hpp"

int main(int /*argc*/, char** /*argv*/) {
    gw::core::log_message(gw::core::LogLevel::Info, "editor",
        "Greywater Editor — Cold Coffee Labs");

    gw::editor::EditorApplication app;
    app.run();
    return 0;
}
