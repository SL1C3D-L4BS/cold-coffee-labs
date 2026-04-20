// runtime/main.cpp — Phase 1 skeleton.
// Full runtime (engine->audio->input->UI->console) lands in Phase 11.

#include "engine/core/version.hpp"
#include <cstdio>

int main() {
    std::fprintf(stdout, "[runtime] %s\n", gw::core::version_string());
    return 0;
}
