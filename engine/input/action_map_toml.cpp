// engine/input/action_map_toml.cpp — minimal TOML subset for input maps.
#include "engine/input/action_map_toml.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace gw::input {

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

// Parse "key = value, key = value" style inline-table content (between { }).
// Returns a map of key → raw-unquoted value strings.
std::unordered_map<std::string, std::string> parse_inline_table(std::string_view s) {
    std::unordered_map<std::string, std::string> out;
    std::string buf(s);
    // Strip outer braces if present.
    auto a = buf.find('{');
    auto b = buf.rfind('}');
    if (a != std::string::npos && b != std::string::npos && b > a) {
        buf = buf.substr(a + 1, b - a - 1);
    }
    // Split by commas not inside quotes.
    std::vector<std::string> parts;
    std::string cur;
    bool in_q = false;
    for (char c : buf) {
        if (c == '"') { in_q = !in_q; cur.push_back(c); continue; }
        if (!in_q && c == ',') { parts.push_back(cur); cur.clear(); continue; }
        cur.push_back(c);
    }
    if (!cur.empty()) parts.push_back(cur);

    for (auto& p : parts) {
        auto eq = p.find('=');
        if (eq == std::string::npos) continue;
        out[trim(std::string_view(p).substr(0, eq))] = unquote(trim(std::string_view(p).substr(eq + 1)));
    }
    return out;
}

ActionOutputKind parse_output_kind(std::string_view s) {
    auto t = unquote(s);
    if (t == "bool")  return ActionOutputKind::Bool;
    if (t == "float") return ActionOutputKind::Float;
    if (t == "vec2")  return ActionOutputKind::Vec2;
    if (t == "vec3")  return ActionOutputKind::Vec3;
    if (t == "u32")   return ActionOutputKind::U32;
    return ActionOutputKind::Bool;
}

std::string output_kind_to_string(ActionOutputKind k) {
    switch (k) {
        case ActionOutputKind::Bool:  return "bool";
        case ActionOutputKind::Float: return "float";
        case ActionOutputKind::Vec2:  return "vec2";
        case ActionOutputKind::Vec3:  return "vec3";
        case ActionOutputKind::U32:   return "u32";
    }
    return "bool";
}

} // namespace

ActionSet* ActionMap::find_set(std::string_view name) noexcept {
    for (auto& s : sets) if (s.name() == name) return &s;
    return nullptr;
}
const ActionSet* ActionMap::find_set(std::string_view name) const noexcept {
    for (const auto& s : sets) if (s.name() == name) return &s;
    return nullptr;
}

// --------------------------------------------------------------------------
// Key/button/axis name tables
// --------------------------------------------------------------------------

std::string keycode_to_string(KeyCode k) {
    switch (k) {
        case KeyCode::A: return "a";
        case KeyCode::B: return "b";
        case KeyCode::C: return "c";
        case KeyCode::D: return "d";
        case KeyCode::E: return "e";
        case KeyCode::F: return "f";
        case KeyCode::G: return "g";
        case KeyCode::H: return "h";
        case KeyCode::I: return "i";
        case KeyCode::J: return "j";
        case KeyCode::K: return "k";
        case KeyCode::L: return "l";
        case KeyCode::M: return "m";
        case KeyCode::N: return "n";
        case KeyCode::O: return "o";
        case KeyCode::P: return "p";
        case KeyCode::Q: return "q";
        case KeyCode::R: return "r";
        case KeyCode::S: return "s";
        case KeyCode::T: return "t";
        case KeyCode::U: return "u";
        case KeyCode::V: return "v";
        case KeyCode::W: return "w";
        case KeyCode::X: return "x";
        case KeyCode::Y: return "y";
        case KeyCode::Z: return "z";
        case KeyCode::Return:    return "return";
        case KeyCode::Escape:    return "escape";
        case KeyCode::Backspace: return "backspace";
        case KeyCode::Tab:       return "tab";
        case KeyCode::Space:     return "space";
        case KeyCode::Up:        return "up";
        case KeyCode::Down:      return "down";
        case KeyCode::Left:      return "left";
        case KeyCode::Right:     return "right";
        case KeyCode::LShift:    return "lshift";
        case KeyCode::LCtrl:     return "lctrl";
        case KeyCode::LAlt:      return "lalt";
        default: return "none";
    }
}

KeyCode keycode_from_string(std::string_view s) {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower.size() == 1 && lower[0] >= 'a' && lower[0] <= 'z') {
        return static_cast<KeyCode>(static_cast<uint16_t>(KeyCode::A) + (lower[0] - 'a'));
    }
    if (lower == "return")    return KeyCode::Return;
    if (lower == "escape")    return KeyCode::Escape;
    if (lower == "backspace") return KeyCode::Backspace;
    if (lower == "tab")       return KeyCode::Tab;
    if (lower == "space")     return KeyCode::Space;
    if (lower == "up")        return KeyCode::Up;
    if (lower == "down")      return KeyCode::Down;
    if (lower == "left")      return KeyCode::Left;
    if (lower == "right")     return KeyCode::Right;
    if (lower == "lshift")    return KeyCode::LShift;
    if (lower == "lctrl")     return KeyCode::LCtrl;
    if (lower == "lalt")      return KeyCode::LAlt;
    return KeyCode::None;
}

std::string gamepad_button_to_string(GamepadButton b) {
    switch (b) {
        case GamepadButton::A: return "a";
        case GamepadButton::B: return "b";
        case GamepadButton::X: return "x";
        case GamepadButton::Y: return "y";
        case GamepadButton::Back:         return "back";
        case GamepadButton::Start:        return "start";
        case GamepadButton::LeftStick:    return "left_stick";
        case GamepadButton::RightStick:   return "right_stick";
        case GamepadButton::LeftShoulder: return "l1";
        case GamepadButton::RightShoulder:return "r1";
        case GamepadButton::DPadUp:       return "dpad_up";
        case GamepadButton::DPadDown:     return "dpad_down";
        case GamepadButton::DPadLeft:     return "dpad_left";
        case GamepadButton::DPadRight:    return "dpad_right";
        default: return "none";
    }
}

GamepadButton gamepad_button_from_string(std::string_view s) {
    auto t = std::string(s);
    if (t == "a") return GamepadButton::A;
    if (t == "b") return GamepadButton::B;
    if (t == "x") return GamepadButton::X;
    if (t == "y") return GamepadButton::Y;
    if (t == "back") return GamepadButton::Back;
    if (t == "start") return GamepadButton::Start;
    if (t == "left_stick")  return GamepadButton::LeftStick;
    if (t == "right_stick") return GamepadButton::RightStick;
    if (t == "l1") return GamepadButton::LeftShoulder;
    if (t == "r1") return GamepadButton::RightShoulder;
    if (t == "dpad_up")    return GamepadButton::DPadUp;
    if (t == "dpad_down")  return GamepadButton::DPadDown;
    if (t == "dpad_left")  return GamepadButton::DPadLeft;
    if (t == "dpad_right") return GamepadButton::DPadRight;
    return static_cast<GamepadButton>(0xFFFFu);
}

std::string gamepad_axis_to_string(GamepadAxis a) {
    switch (a) {
        case GamepadAxis::LeftX:  return "left_stick_x";
        case GamepadAxis::LeftY:  return "left_stick_y";
        case GamepadAxis::RightX: return "right_stick_x";
        case GamepadAxis::RightY: return "right_stick_y";
        case GamepadAxis::TriggerLeft:  return "l2";
        case GamepadAxis::TriggerRight: return "r2";
        default: return "none";
    }
}
GamepadAxis gamepad_axis_from_string(std::string_view s) {
    if (s == "left_stick")   return GamepadAxis::LeftX;  // shorthand
    if (s == "right_stick")  return GamepadAxis::RightX;
    if (s == "left_stick_x") return GamepadAxis::LeftX;
    if (s == "left_stick_y") return GamepadAxis::LeftY;
    if (s == "right_stick_x")return GamepadAxis::RightX;
    if (s == "right_stick_y")return GamepadAxis::RightY;
    if (s == "l2") return GamepadAxis::TriggerLeft;
    if (s == "r2") return GamepadAxis::TriggerRight;
    return static_cast<GamepadAxis>(0xFFFFu);
}

// --------------------------------------------------------------------------
// Binding (de)serialization
// --------------------------------------------------------------------------

std::string binding_to_string(const Binding& b) {
    std::ostringstream ss;
    ss << "{ ";
    switch (b.source) {
        case BindingSource::Keyboard:
            ss << "device = \"keyboard\", input = \""
               << keycode_to_string(static_cast<KeyCode>(b.code)) << "\"";
            break;
        case BindingSource::Mouse:
            ss << "device = \"mouse\", input = \"button_";
            switch (b.code) {
                case static_cast<uint32_t>(MouseButton::Left):   ss << "left"; break;
                case static_cast<uint32_t>(MouseButton::Right):  ss << "right"; break;
                case static_cast<uint32_t>(MouseButton::Middle): ss << "middle"; break;
                default: ss << b.code; break;
            }
            ss << "\"";
            break;
        case BindingSource::MouseAxis:
            ss << "device = \"mouse\", input = \"axis_" << b.code << "\"";
            break;
        case BindingSource::GamepadButton:
            ss << "device = \"gamepad\", input = \""
               << gamepad_button_to_string(static_cast<GamepadButton>(b.code)) << "\"";
            break;
        case BindingSource::GamepadAxis:
            ss << "device = \"gamepad\", input = \""
               << gamepad_axis_to_string(static_cast<GamepadAxis>(b.code)) << "\"";
            if (b.threshold != 0.0f) ss << ", threshold = " << b.threshold;
            break;
        case BindingSource::CompositeWASD:
            ss << "device = \"keyboard\", input = \"composite_wasd\"";
            break;
        case BindingSource::GamepadGyro:
            ss << "device = \"gamepad\", input = \"gyro_" << b.code << "\"";
            break;
        default:
            ss << "device = \"none\", input = \"none\"";
            break;
    }
    ss << " }";
    return ss.str();
}

Binding binding_from_string(const std::string& s) {
    Binding b{};
    auto fields = parse_inline_table(s);
    const auto& dev = fields["device"];
    const auto& inp = fields["input"];

    if (dev == "keyboard") {
        if (inp == "composite_wasd") {
            b.source = BindingSource::CompositeWASD;
            // Default WASD
            b.composite_keys[0] = static_cast<uint16_t>(KeyCode::W);
            b.composite_keys[1] = static_cast<uint16_t>(KeyCode::S);
            b.composite_keys[2] = static_cast<uint16_t>(KeyCode::A);
            b.composite_keys[3] = static_cast<uint16_t>(KeyCode::D);
        } else {
            b.source = BindingSource::Keyboard;
            b.code = static_cast<uint32_t>(keycode_from_string(inp));
        }
    } else if (dev == "mouse") {
        if (inp.rfind("button_", 0) == 0) {
            b.source = BindingSource::Mouse;
            std::string rest = inp.substr(7);
            if (rest == "left")        b.code = 1;
            else if (rest == "right")  b.code = 2;
            else if (rest == "middle") b.code = 3;
            else                       b.code = static_cast<uint32_t>(std::atoi(rest.c_str()));
        } else if (inp.rfind("axis_", 0) == 0) {
            b.source = BindingSource::MouseAxis;
            b.code = static_cast<uint32_t>(std::atoi(inp.c_str() + 5));
        }
    } else if (dev == "gamepad") {
        auto btn = gamepad_button_from_string(inp);
        if (btn != static_cast<GamepadButton>(0xFFFFu)) {
            b.source = BindingSource::GamepadButton;
            b.code = static_cast<uint32_t>(btn);
        } else {
            auto ax = gamepad_axis_from_string(inp);
            if (ax != static_cast<GamepadAxis>(0xFFFFu)) {
                b.source = BindingSource::GamepadAxis;
                b.code = static_cast<uint32_t>(ax);
            } else if (inp.rfind("gyro_", 0) == 0) {
                b.source = BindingSource::GamepadGyro;
                b.code = static_cast<uint32_t>(std::atoi(inp.c_str() + 5));
            }
        }
    }

    auto it = fields.find("threshold");
    if (it != fields.end()) {
        try { b.threshold = std::stof(it->second); } catch (...) {}
    }
    return b;
}

// --------------------------------------------------------------------------
// Action-map load / save
// --------------------------------------------------------------------------

bool load_action_map_toml(const std::string& source, ActionMap& out) {
    out.sets.clear();
    out.defaults = {};

    std::istringstream ss(source);
    std::string line;
    std::string section;
    ActionSet*  current_set = nullptr;
    Action*     current_action = nullptr;

    auto get_or_create_set = [&](std::string_view name) -> ActionSet* {
        if (auto* s = out.find_set(name); s) return s;
        out.sets.emplace_back(std::string(name));
        return &out.sets.back();
    };

    // Default set is "gameplay" if the file doesn't declare one explicitly.
    current_set = get_or_create_set("gameplay");

    while (std::getline(ss, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);
        auto t = trim(line);
        if (t.empty()) continue;

        if (t.front() == '[' && t.back() == ']') {
            section = t.substr(1, t.size() - 2);
            if (section == "accessibility") {
                current_action = nullptr;
                continue;
            }
            if (section.rfind("set.", 0) == 0) {
                current_set = get_or_create_set(section.substr(4));
                current_action = nullptr;
                continue;
            }
            if (section.rfind("action.", 0) == 0) {
                Action a{};
                a.name = section.substr(7);
                current_action = &current_set->add(a);
                continue;
            }
            continue;
        }

        auto eq = t.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(std::string_view(t).substr(0, eq));
        std::string val = trim(std::string_view(t).substr(eq + 1));

        if (section == "accessibility") {
            if (key == "hold_to_toggle")             out.defaults.hold_to_toggle = (unquote(val) == "true");
            else if (key == "repeat_cooldown_ms") {
                try { out.defaults.repeat_cooldown_ms = std::stof(val); } catch (...) {}
            }
            else if (key == "single_switch_scan_rate_ms") {
                // Stored on defaults.auto_repeat_ms as the scan rate seed; the scanner
                // reads its own config elsewhere.
                try { out.defaults.auto_repeat_ms = std::stof(val); } catch (...) {}
            }
            continue;
        }

        if (current_action == nullptr) continue;
        if (key == "output") {
            current_action->output = parse_output_kind(val);
        } else if (key == "bindings") {
            // Array of inline tables — e.g. bindings = [ { device=... }, { ... } ]
            // We split by top-level commas between matching braces.
            std::string s = val;
            // Peel outer [ ]
            auto lb = s.find('[');
            auto rb = s.rfind(']');
            if (lb != std::string::npos && rb != std::string::npos) {
                s = s.substr(lb + 1, rb - lb - 1);
            }
            // Walk characters; collect each { ... } substring.
            int depth = 0;
            std::string cur;
            for (char c : s) {
                if (c == '{') { if (depth == 0) cur.clear(); ++depth; cur.push_back(c); }
                else if (c == '}') { cur.push_back(c); --depth; if (depth == 0) {
                    current_action->bindings.push_back(binding_from_string(cur));
                    cur.clear();
                }}
                else if (depth > 0) cur.push_back(c);
            }
        } else if (key == "hold_to_toggle") {
            current_action->accessibility.hold_to_toggle = (unquote(val) == "true");
        } else if (key == "auto_repeat_ms") {
            try { current_action->accessibility.auto_repeat_ms = std::stof(val); } catch (...) {}
        } else if (key == "repeat_cooldown_ms") {
            try { current_action->accessibility.repeat_cooldown_ms = std::stof(val); } catch (...) {}
        } else if (key == "assist_window_ms") {
            try { current_action->accessibility.assist_window_ms = std::stof(val); } catch (...) {}
        } else if (key == "scan_index") {
            try { current_action->accessibility.scan_index = std::stoi(val); } catch (...) {}
        } else if (key == "can_be_invoked_while_paused") {
            current_action->accessibility.can_be_invoked_while_paused = (unquote(val) == "true");
        }
    }
    // Apply defaults where an Action did not specify its own flag.
    for (auto& s : out.sets) {
        for (auto& a : s.actions()) {
            if (out.defaults.hold_to_toggle && !a.accessibility.hold_to_toggle) {
                a.accessibility.hold_to_toggle = true;
            }
            if (a.accessibility.repeat_cooldown_ms == 0.0f && out.defaults.repeat_cooldown_ms > 0.0f) {
                a.accessibility.repeat_cooldown_ms = out.defaults.repeat_cooldown_ms;
            }
        }
    }
    return true;
}

std::string save_action_map_toml(const ActionMap& map) {
    std::ostringstream ss;
    ss << "# Greywater input map — Phase 10 TOML round-trip\n";
    ss << "[accessibility]\n";
    ss << "hold_to_toggle = " << (map.defaults.hold_to_toggle ? "true" : "false") << "\n";
    ss << "repeat_cooldown_ms = " << map.defaults.repeat_cooldown_ms << "\n";
    ss << "\n";

    for (const auto& set : map.sets) {
        if (set.name() != "gameplay") {
            ss << "[set." << set.name() << "]\n\n";
        }
        for (const auto& a : set.actions()) {
            ss << "[action." << a.name << "]\n";
            ss << "output = \"" << output_kind_to_string(a.output) << "\"\n";
            // Emit bindings on a single line so the line-oriented loader can
            // round-trip them without multi-line state.
            ss << "bindings = [";
            for (std::size_t i = 0; i < a.bindings.size(); ++i) {
                if (i) ss << ", ";
                ss << binding_to_string(a.bindings[i]);
            }
            ss << "]\n";
            if (a.accessibility.hold_to_toggle)          ss << "hold_to_toggle = true\n";
            if (a.accessibility.auto_repeat_ms > 0.0f)   ss << "auto_repeat_ms = " << a.accessibility.auto_repeat_ms << "\n";
            if (a.accessibility.repeat_cooldown_ms > 0.0f) ss << "repeat_cooldown_ms = " << a.accessibility.repeat_cooldown_ms << "\n";
            if (a.accessibility.assist_window_ms > 0.0f) ss << "assist_window_ms = " << a.accessibility.assist_window_ms << "\n";
            if (a.accessibility.scan_index >= 0)         ss << "scan_index = " << a.accessibility.scan_index << "\n";
            if (a.accessibility.can_be_invoked_while_paused) ss << "can_be_invoked_while_paused = true\n";
            ss << "\n";
        }
    }
    return ss.str();
}

} // namespace gw::input
