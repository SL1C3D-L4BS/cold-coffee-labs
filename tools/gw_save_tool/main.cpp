// tools/gw_save_tool — validate `.gwsave` header + footer + dry-run migrate (ADR-0064).
//
// Usage:
//   gw_save_tool <file.gwsave>                      -- validate header + BLAKE3 footer
//   gw_save_tool validate <file.gwsave> [...]       -- validate one or more files
//   gw_save_tool migrate --dry-run <file.gwsave>... -- report needed migrations, no writes
//
// All modes exit 0 on success, 1 on any file failing validation.

#include "engine/persist/gwsave_io.hpp"
#include "engine/persist/persist_types.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

int validate_one(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "gw_save_tool: cannot open " << path << '\n';
        return 1;
    }
    f.seekg(0, std::ios::end);
    const auto sz = f.tellg();
    f.seekg(0);
    if (sz <= 0) {
        std::cerr << "gw_save_tool: empty " << path << '\n';
        return 1;
    }
    std::vector<std::byte> buf(static_cast<std::size_t>(sz));
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    const auto peek = gw::persist::peek_gwsave(
        std::span<const std::byte>(buf.data(), buf.size()), true);
    if (peek.err != gw::persist::LoadError::Ok) {
        std::cerr << "gw_save_tool: " << path << ": "
                  << gw::persist::to_string(peek.err) << '\n';
        return 1;
    }
    std::cout << "ok " << path
              << " schema=" << peek.hdr.schema_version
              << " engine=" << peek.hdr.engine_version
              << " chunks=" << peek.hdr.chunk_count
              << " flags=" << peek.hdr.flags
              << '\n';
    return 0;
}

int migrate_dry_run_one(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "gw_save_tool: cannot open " << path << '\n';
        return 1;
    }
    f.seekg(0, std::ios::end);
    const auto sz = f.tellg();
    f.seekg(0);
    std::vector<std::byte> buf(static_cast<std::size_t>(sz));
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    const auto peek = gw::persist::peek_gwsave(
        std::span<const std::byte>(buf.data(), buf.size()), true);
    if (peek.err != gw::persist::LoadError::Ok) {
        std::cerr << "gw_save_tool: " << path << ": "
                  << gw::persist::to_string(peek.err) << '\n';
        return 1;
    }
    const std::uint32_t from = peek.hdr.schema_version;
    const std::uint32_t to   = gw::persist::kCurrentSaveSchemaVersion;
    if (from == to) {
        std::cout << "current " << path << " schema=" << from << '\n';
    } else {
        std::cout << "migrate " << path
                  << " from=" << from << " to=" << to
                  << " (dry-run, no write)\n";
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage:\n"
                     "  gw_save_tool <file.gwsave>\n"
                     "  gw_save_tool validate <file.gwsave>...\n"
                     "  gw_save_tool migrate --dry-run <file.gwsave>...\n";
        return 2;
    }

    const std::string_view a1{argv[1]};
    if (a1 == "migrate") {
        if (argc < 4 || std::string_view{argv[2]} != "--dry-run") {
            std::cerr << "gw_save_tool: migrate requires --dry-run <files>\n";
            return 2;
        }
        int rc = 0;
        for (int i = 3; i < argc; ++i) rc |= migrate_dry_run_one(argv[i]);
        return rc ? 1 : 0;
    }
    if (a1 == "validate") {
        int rc = 0;
        for (int i = 2; i < argc; ++i) rc |= validate_one(argv[i]);
        return rc ? 1 : 0;
    }
    return validate_one(argv[1]);
}
