// engine/core/config/toml_binding.cpp — ADR-0024 TOML round-trip.
//
// Hand-rolled TOML subset reader/writer. Supports scalar values, nested
// table headers, comments, quoted strings, and bool/int/float/string.
// Lines are trimmed; blank lines ignored.

#include "engine/core/config/toml_binding.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace gw::config {

namespace {

std::string_view ltrim(std::string_view s) {
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    return s.substr(i);
}
std::string_view rtrim(std::string_view s) {
    std::size_t n = s.size();
    while (n > 0 && std::isspace(static_cast<unsigned char>(s[n - 1]))) --n;
    return s.substr(0, n);
}
std::string_view trim(std::string_view s) { return rtrim(ltrim(s)); }

// Split a toml value on the first unescaped '#'.
std::string_view strip_comment(std::string_view v) {
    bool in_q = false;
    char q = 0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        char c = v[i];
        if (in_q) {
            if (c == '\\' && i + 1 < v.size()) { ++i; continue; }
            if (c == q) in_q = false;
        } else {
            if (c == '"' || c == '\'') { in_q = true; q = c; continue; }
            if (c == '#') return v.substr(0, i);
        }
    }
    return v;
}

std::string unquote(std::string_view s) {
    s = trim(s);
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"')
                       || (s.front() == '\'' && s.back() == '\''))) {
        s = s.substr(1, s.size() - 2);
    }
    // Minimal unescape for "\n", "\t", "\"", "\\".
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case 'n': out.push_back('\n'); ++i; break;
                case 't': out.push_back('\t'); ++i; break;
                case '"': out.push_back('"');  ++i; break;
                case '\\': out.push_back('\\'); ++i; break;
                default:  out.push_back(s[i]);     break;
            }
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

// Convert parsed raw value to a canonical string acceptable by
// CVarRegistry::set_from_string. Chooses between string / bool / number
// based on the presence of quotes + common patterns.
std::string canonicalise_value(std::string_view raw) {
    auto v = trim(strip_comment(raw));
    if (v.empty()) return {};
    if (v.front() == '"' || v.front() == '\'') {
        return unquote(v);
    }
    // bool / numeric / otherwise — leave as-is.
    return std::string{v};
}

bool is_string_value(std::string_view raw) {
    auto v = trim(strip_comment(raw));
    return !v.empty() && (v.front() == '"' || v.front() == '\'');
}

} // namespace

core::Result<std::uint32_t, TomlError>
load_domain_toml(std::string_view src, std::string_view prefix, CVarRegistry& registry) {
    std::uint32_t line = 0;
    std::uint32_t applied = 0;
    std::string   current_table{};
    std::size_t   pos = 0;
    while (pos < src.size()) {
        ++line;
        // Read a line.
        std::size_t nl = src.find('\n', pos);
        std::string_view raw = src.substr(pos, (nl == std::string_view::npos ? src.size() : nl) - pos);
        pos = (nl == std::string_view::npos ? src.size() : nl + 1);
        auto l = trim(raw);
        if (l.empty() || l.front() == '#') continue;
        if (l.front() == '[') {
            auto rb = l.find(']');
            if (rb == std::string_view::npos) {
                return TomlError{"unterminated table header", line, 1};
            }
            current_table = std::string{trim(l.substr(1, rb - 1))};
            continue;
        }
        auto eq = l.find('=');
        if (eq == std::string_view::npos) {
            return TomlError{"missing '=' in key/value", line, 1};
        }
        std::string key{trim(l.substr(0, eq))};
        std::string_view raw_val = l.substr(eq + 1);
        std::string full_name;
        if (!current_table.empty()) {
            full_name = current_table + "." + key;
        } else {
            full_name = key;
        }
        if (!prefix.empty() && full_name.rfind(prefix, 0) != 0) {
            // Silently ignore keys from other domains rather than erroring —
            // a user may have pasted several domain sections into one file.
            continue;
        }
        std::string value = canonicalise_value(raw_val);
        const bool as_string = is_string_value(raw_val);
        // Hint: if the target type is an enum and the value is unquoted,
        // treat it as a label; otherwise set_from_string handles both.
        (void)as_string;
        auto rc = registry.set_from_string(full_name, value, kOriginTomlLoad);
        switch (rc) {
            case CVarRegistry::SetResult::Ok:
                ++applied;
                break;
            case CVarRegistry::SetResult::NotFound:
                // Unknown CVar: tolerate (forward-compat for older saves).
                break;
            case CVarRegistry::SetResult::TypeMismatch:
                return TomlError{"type mismatch for '" + full_name + "'", line, 1};
            case CVarRegistry::SetResult::ParseError:
                return TomlError{"parse error for '" + full_name + "'", line, 1};
            case CVarRegistry::SetResult::OutOfRange:
                // Clamp was already applied on numeric; treat as warning-like.
                ++applied;
                break;
            case CVarRegistry::SetResult::ReadOnly:
                // Accept silently: .toml files can't override read-only.
                break;
        }
    }
    return applied;
}

// --- save -----------------------------------------------------------------

namespace {

void append_escaped_string(std::string& out, std::string_view s) {
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned int>(c));
                    out += buf;
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    out.push_back('"');
}

std::string render_value(const CVarEntry& e) {
    std::string v;
    switch (e.type) {
        case CVarType::Bool:   v = e.v_bool ? "true" : "false"; break;
        case CVarType::Int32:  v = std::to_string(e.v_i32); break;
        case CVarType::Int64:  v = std::to_string(e.v_i64); break;
        case CVarType::Float: {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.9g", static_cast<double>(e.v_f32));
            v = buf;
            break;
        }
        case CVarType::Double: {
            char buf[40];
            std::snprintf(buf, sizeof(buf), "%.17g", e.v_f64);
            v = buf;
            break;
        }
        case CVarType::String: append_escaped_string(v, e.v_str); break;
        case CVarType::Enum: {
            for (const auto& ev : e.enum_variants) {
                if (ev.value == e.v_i32) {
                    append_escaped_string(v, ev.label);
                    return v;
                }
            }
            v = std::to_string(e.v_i32);
            break;
        }
    }
    return v;
}

} // namespace

std::string save_domain_toml(std::string_view prefix, const CVarRegistry& registry) {
    std::vector<const CVarEntry*> eligible;
    eligible.reserve(registry.count());
    for (const auto& e : registry.entries()) {
        if (!(e.flags & kCVarPersist)) continue;
        if (!prefix.empty() && e.name.rfind(prefix, 0) != 0) continue;
        eligible.push_back(&e);
    }
    std::sort(eligible.begin(), eligible.end(),
              [](const CVarEntry* a, const CVarEntry* b) { return a->name < b->name; });

    std::string out;
    out.reserve(eligible.size() * 32);
    out += "# Greywater config — generated. Hand-edits preserved on reload.\n";
    if (!prefix.empty()) {
        out += "# Domain: ";
        out.append(prefix.data(), prefix.size());
        out += "\n";
    }
    std::string current_table;
    for (const auto* e : eligible) {
        auto dot = e->name.rfind('.');
        std::string tbl, key;
        if (dot == std::string::npos) {
            tbl.clear();
            key = e->name;
        } else {
            tbl = e->name.substr(0, dot);
            key = e->name.substr(dot + 1);
        }
        if (tbl != current_table) {
            out += "\n[";
            out += tbl;
            out += "]\n";
            current_table = tbl;
        }
        out += key;
        out += " = ";
        out += render_value(*e);
        if (!e->description.empty()) {
            out += "  # ";
            out += e->description;
        }
        out += "\n";
    }
    return out;
}

} // namespace gw::config
