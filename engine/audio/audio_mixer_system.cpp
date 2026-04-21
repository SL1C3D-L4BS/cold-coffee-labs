// engine/audio/audio_mixer_system.cpp
#include "engine/audio/audio_mixer_system.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace gw::audio {

namespace {

std::string trim(std::string_view s) {
    std::size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    std::size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return std::string(s.substr(a, b - a));
}

std::string unquote(std::string_view s) {
    auto t = trim(s);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        return t.substr(1, t.size() - 2);
    }
    return t;
}

bool parse_when(const std::string& raw, MixingRule& r) {
    // Expected: "field op value"
    std::string s = unquote(raw);
    // Find the operator.
    static const std::pair<std::string, CondOp> ops[] = {
        {">=", CondOp::GreaterEq},
        {"<=", CondOp::LessEq},
        {"==", CondOp::Eq},
        {">",  CondOp::Greater},
        {"<",  CondOp::Less},
    };
    std::size_t op_pos = std::string::npos;
    CondOp op = CondOp::Greater;
    std::size_t op_len = 0;
    for (auto& pr : ops) {
        auto p = s.find(pr.first);
        if (p != std::string::npos) {
            op_pos = p;
            op = pr.second;
            op_len = pr.first.size();
            break;
        }
    }
    if (op_pos == std::string::npos) return false;

    r.field = trim(std::string_view(s).substr(0, op_pos));
    r.op = op;
    std::string rhs = trim(std::string_view(s).substr(op_pos + op_len));
    try {
        r.threshold = std::stof(rhs);
    } catch (...) {
        return false;
    }
    return true;
}

float field_value(const GameState& g, const std::string& name) noexcept {
    if (name == "combat_intensity")    return g.combat_intensity;
    if (name == "atmospheric_density") return g.atmospheric_density;
    if (name == "player_health_pct")   return g.player_health_pct;
    if (name == "in_menu")             return g.in_menu ? 1.0f : 0.0f;
    if (name == "in_vehicle")          return g.in_vehicle ? 1.0f : 0.0f;
    if (name == "in_space")            return g.in_space ? 1.0f : 0.0f;
    return 0.0f;
}

bool eval_condition(const MixingRule& r, const GameState& g) noexcept {
    const float v = field_value(g, r.field);
    switch (r.op) {
        case CondOp::Greater:   return v >  r.threshold;
        case CondOp::Less:      return v <  r.threshold;
        case CondOp::GreaterEq: return v >= r.threshold;
        case CondOp::LessEq:    return v <= r.threshold;
        case CondOp::Eq:        return v == r.threshold;
    }
    return false;
}

} // namespace

BusId AudioMixerSystem::bus_from_string(const std::string& s) noexcept {
    if (s == "master")              return BusId::Master;
    if (s == "sfx")                 return BusId::SFX;
    if (s == "sfx.weapons")         return BusId::SFX_Weapons;
    if (s == "sfx.environment")     return BusId::SFX_Environment;
    if (s == "sfx.ui")              return BusId::SFX_UI;
    if (s == "music")               return BusId::Music;
    if (s == "music.combat")        return BusId::Music_Combat;
    if (s == "music.exploration")   return BusId::Music_Exploration;
    if (s == "music.space")         return BusId::Music_Space;
    if (s == "dialogue")            return BusId::Dialogue;
    if (s == "dialogue.npc")        return BusId::Dialogue_NPC;
    if (s == "dialogue.radio")      return BusId::Dialogue_Radio;
    if (s == "ambience")            return BusId::Ambience;
    if (s == "ambience.wind")       return BusId::Ambience_Wind;
    if (s == "ambience.water")      return BusId::Ambience_Water;
    if (s == "ambience.space")      return BusId::Ambience_Space;
    return BusId::Master;
}

std::string AudioMixerSystem::bus_to_string(BusId b) noexcept {
    switch (b) {
        case BusId::Master:              return "master";
        case BusId::SFX:                 return "sfx";
        case BusId::SFX_Weapons:         return "sfx.weapons";
        case BusId::SFX_Environment:     return "sfx.environment";
        case BusId::SFX_UI:              return "sfx.ui";
        case BusId::Music:               return "music";
        case BusId::Music_Combat:        return "music.combat";
        case BusId::Music_Exploration:   return "music.exploration";
        case BusId::Music_Space:         return "music.space";
        case BusId::Dialogue:            return "dialogue";
        case BusId::Dialogue_NPC:        return "dialogue.npc";
        case BusId::Dialogue_Radio:      return "dialogue.radio";
        case BusId::Ambience:            return "ambience";
        case BusId::Ambience_Wind:       return "ambience.wind";
        case BusId::Ambience_Water:      return "ambience.water";
        case BusId::Ambience_Space:      return "ambience.space";
        default:                         return "master";
    }
}

bool AudioMixerSystem::parse_toml(const std::string& source, std::vector<MixingRule>& rules) {
    rules.clear();
    std::istringstream ss(source);
    std::string line;
    std::string current_section;
    MixingRule current{};
    bool have_current = false;

    auto commit = [&]() {
        if (have_current && !current.field.empty() && !current.name.empty()) {
            rules.push_back(current);
        }
        current = MixingRule{};
        have_current = false;
    };

    while (std::getline(ss, line)) {
        // Strip comments (# ...)
        auto hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);
        auto t = trim(line);
        if (t.empty()) continue;

        if (t.front() == '[' && t.back() == ']') {
            commit();
            auto section = t.substr(1, t.size() - 2);
            current_section = section;
            // Support [rule.xyz]
            if (section.rfind("rule.", 0) == 0) {
                current = MixingRule{};
                current.name = section.substr(5);
                have_current = true;
            }
            continue;
        }

        if (!have_current) continue;

        auto eq = t.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(std::string_view(t).substr(0, eq));
        std::string val = trim(std::string_view(t).substr(eq + 1));

        if (key == "when") {
            if (!parse_when(val, current)) return false;
        } else if (key == "bus") {
            current.bus = bus_from_string(unquote(val));
        } else if (key == "delta_db") {
            try { current.delta_db = std::stof(val); } catch (...) { return false; }
        } else if (key == "mute") {
            auto v = unquote(val);
            current.mute_when_active = (v == "true" || v == "1");
        }
    }
    commit();
    return true;
}

void AudioMixerSystem::apply(const std::vector<MixingRule>& rules,
                             const GameState& state,
                             AudioService& service) {
    for (const auto& r : rules) {
        if (!eval_condition(r, state)) continue;
        if (r.mute_when_active) {
            service.set_bus_mute(r.bus, true);
        } else {
            service.duck_bus(r.bus, r.delta_db);
        }
    }
}

} // namespace gw::audio
