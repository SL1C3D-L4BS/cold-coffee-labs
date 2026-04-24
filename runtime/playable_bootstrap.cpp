// runtime/playable_bootstrap.cpp

#include "runtime/playable_bootstrap.hpp"
#include "runtime/engine.hpp"

#include "engine/play/play_bootstrap_cvars.hpp"

#include <cctype>
#include <cstdlib>
#include <string_view>

namespace gw::runtime {
namespace {

bool starts_with(std::string_view s, std::string_view p) noexcept {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

[[nodiscard]] std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
    return s;
}

[[nodiscard]] const char* getenv_str(const char* k) noexcept {
    if (!k) return nullptr;
#if defined(_WIN32)
    return std::getenv(k);
#else
    return std::getenv(k);
#endif
}

} // namespace

void parse_playable_cli(int argc, char** argv, PlayableCliOptions& out) {
    for (int i = 1; i < argc; ++i) {
        std::string_view a{argv[i]};
        if (starts_with(a, "--scene=")) {
            out.scene_path = std::string{a.substr(8)};
            continue;
        }
        if (starts_with(a, "--seed=")) {
            try {
                out.universe_seed = static_cast<std::int64_t>(
                    std::stoll(std::string{a.substr(7)}));
            } catch (...) {
            }
            continue;
        }
        if (starts_with(a, "--cvars-toml=")) {
            out.cvars_toml_path = std::string{a.substr(13)};
            continue;
        }
    }

    if (out.scene_path.empty()) {
        if (const char* e = getenv_str("GW_PLAY_SCENE")) {
            out.scene_path = trim(std::string_view{e});
        }
    }
    if (!out.universe_seed.has_value()) {
        if (const char* e = getenv_str("GW_UNIVERSE_SEED")) {
            try {
                out.universe_seed =
                    static_cast<std::int64_t>(std::stoll(std::string{trim(e)}));
            } catch (...) {
            }
        }
    }
    if (out.cvars_toml_path.empty()) {
        if (const char* e = getenv_str("GW_CVARS_TOML")) {
            out.cvars_toml_path = trim(std::string_view{e});
        }
    }
}

void apply_playable_bootstrap(Engine& engine, const PlayableCliOptions& cli) {
    gw::play::apply_play_bootstrap_to_registry(engine.cvars(), cli.universe_seed,
                                              cli.cvars_toml_path);
}

} // namespace gw::runtime
