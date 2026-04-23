// editor/shell/private_gate.cpp

#include "editor/shell/private_gate.hpp"
#include "editor/config/editor_paths.hpp"
#include "editor/shell/sha256.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace gw::editor::shell {
namespace {

[[nodiscard]] std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

struct GateFile {
    std::string user_expect;
    std::string password_plain;
    std::string salt;
    std::string password_sha256_hex;
};

[[nodiscard]] bool hex_eq_digest(std::string_view hex,
                                 const std::uint8_t digest[32]) noexcept {
    if (hex.size() != 64) return false;
    char computed_hex[65];
    static_cast<void>(digest_to_hex(digest, computed_hex));
    for (int i = 0; i < 64; ++i) {
        char a = hex[static_cast<std::size_t>(i)];
        char b = computed_hex[i];
        if (a >= 'A' && a <= 'F') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'F') b = static_cast<char>(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}

[[nodiscard]] bool load_gate_file(GateFile& out) noexcept {
    namespace fs = std::filesystem;
    const fs::path path = config::editor_data_root() / "private_gate.toml";
    std::ifstream in{path};
    if (!in.good()) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
            val = val.substr(1, val.size() - 2);

        if (key == "user") out.user_expect = val;
        else if (key == "password_plain") out.password_plain = val;
        else if (key == "salt") out.salt = val;
        else if (key == "password_sha256_hex") out.password_sha256_hex = val;
    }
    return true;
}

} // namespace

bool env_skip_private_gate() noexcept {
#if defined(GREYWATER_EDITOR_SKIP_GATE) && GREYWATER_EDITOR_SKIP_GATE
    return true;
#else
    if (const char* e = std::getenv("GREYWATER_EDITOR_SKIP_GATE"); e &&
        e[0] != '\0' && std::strcmp(e, "0") != 0)
        return true;
    return false;
#endif
}

bool private_gate_file_exists() noexcept {
    namespace fs = std::filesystem;
    return fs::exists(config::editor_data_root() / "private_gate.toml");
}

bool try_private_gate(std::string_view username, std::string_view password,
                      char* err, std::size_t err_cap) noexcept {
    if (err_cap > 0) err[0] = '\0';

    GateFile gf{};
    if (!load_gate_file(gf)) {
        if (err_cap > 0)
            std::snprintf(err, err_cap, "Gate file missing or unreadable.");
        return false;
    }

    if (!gf.user_expect.empty() && gf.user_expect != username) {
        if (err_cap > 0) std::snprintf(err, err_cap, "Identity mismatch.");
        return false;
    }

    if (!gf.password_plain.empty()) {
        if (gf.password_plain != password) {
            if (err_cap > 0) std::snprintf(err, err_cap, "Invalid passphrase.");
            return false;
        }
        return true;
    }

    if (gf.password_sha256_hex.size() == 64 && !gf.salt.empty()) {
        std::string combined;
        combined.reserve(gf.salt.size() + password.size());
        combined.append(gf.salt);
        combined.append(password);

        std::uint8_t digest[32];
        sha256(combined, digest);
        if (!hex_eq_digest(gf.password_sha256_hex, digest)) {
            if (err_cap > 0) std::snprintf(err, err_cap, "Invalid passphrase.");
            return false;
        }
        return true;
    }

    if (err_cap > 0)
        std::snprintf(err, err_cap,
                      "Configure password_plain or salt+password_sha256_hex.");
    return false;
}

} // namespace gw::editor::shell
