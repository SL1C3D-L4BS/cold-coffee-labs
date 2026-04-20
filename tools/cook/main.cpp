// tools/cook/main.cpp — Phase 1 skeleton. Phase 6 implements the real cooker.

#include "engine/core/version.hpp"
#include <cstdio>
#include <cstring>

static void print_usage() {
    std::fprintf(stdout,
        "gw_cook — Greywater content cooker (Phase 1 skeleton)\n"
        "Usage:\n"
        "  gw_cook --version\n"
        "  gw_cook --help\n"
        "\n"
        "Phase 6 implements:\n"
        "  gw_cook --input <dir> --output <dir> [--incremental] [--jobs N]\n");
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        print_usage();
        return 0;
    }
    if (std::strcmp(argv[1], "--version") == 0) {
        std::fprintf(stdout, "%s\n", gw::core::version_string());
        return 0;
    }
    print_usage();
    return 0;
}
