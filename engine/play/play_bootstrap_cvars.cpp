// engine/play/play_bootstrap_cvars.cpp

#include "engine/play/play_bootstrap_cvars.hpp"

#include "engine/core/config/cvar.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/toml_binding.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace gw::play {

void apply_play_bootstrap_to_registry(gw::config::CVarRegistry&     reg,
                                      std::optional<std::int64_t>   universe_seed,
                                      std::string_view              cvars_toml_abs_utf8) {
    if (universe_seed.has_value()) {
        std::ostringstream oss;
        oss << *universe_seed;
        (void)reg.set_from_string("rng.seed", oss.str(), gw::config::kOriginProgrammatic);
    }

    if (cvars_toml_abs_utf8.empty()) {
        return;
    }

    const std::string path{cvars_toml_abs_utf8};
    std::ifstream     in(path, std::ios::binary);
    if (!in) {
        return;
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    (void)gw::config::load_domain_toml(buf.str(), "", reg);
}

} // namespace gw::play
