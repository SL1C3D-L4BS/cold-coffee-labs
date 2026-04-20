// editor/main.cpp — Greywater editor, Phase 1 *First Light* Milestone 1B.
//
// The full editor — ImGui docking + viewports + ImNodes + ImGuizmo + BLD chat
// panel — lands in Phase 7 (docs/05, docs/architecture/…#3.31, Chapter 13).
// This Phase 1 binary proves the C++ <-> Rust boundary links: greywater_core
// (C++23 static lib) + bld-bridge (Rust static lib via Corrosion) + platform
// loader libraries, all under Clang on both Windows and Linux.

#include "editor/bld_bridge/hello.hpp"
#include "engine/core/version.hpp"

#include <cstdio>

int main() {
    std::fprintf(stdout, "[editor] %s\n", gw::core::version_string());
    const char* greeting = gw::editor::bld_greeting();
    if (greeting == nullptr) {
        std::fprintf(stderr, "[editor] bld_hello returned null — FFI broken\n");
        return 1;
    }
    std::fprintf(stdout, "[editor] %s\n", greeting);
    return 0;
}
