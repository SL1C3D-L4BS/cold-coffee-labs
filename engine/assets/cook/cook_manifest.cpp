#include "cook_manifest.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>

namespace gw::assets::cook {

static std::string hex128(const CookKey& k) {
    char buf[33]{};
    std::snprintf(buf, sizeof(buf), "%016llx%016llx",
                  static_cast<unsigned long long>(k.hi),
                  static_cast<unsigned long long>(k.lo));
    return buf;
}

static std::string json_string(const std::string& s) {
    // Minimal JSON string escaping.
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    out += '"';
    return out;
}

std::string CookManifest::to_json() const {
    std::ostringstream o;
    o << "{\n";
    o << "  \"cook_version\": " << cook_version << ",\n";
    o << "  \"platform\": " << json_string(platform) << ",\n";
    o << "  \"config\": " << json_string(config) << ",\n";
    o << "  \"timestamp\": " << timestamp << ",\n";
    o << "  \"engine_version\": " << json_string(engine_version) << ",\n";
    o << "  \"assets\": [\n";
    for (std::size_t i = 0; i < assets.size(); ++i) {
        const auto& a = assets[i];
        o << "    {\n";
        o << "      \"source\": " << json_string(a.source) << ",\n";
        o << "      \"output\": " << json_string(a.output) << ",\n";
        o << "      \"cook_key\": " << json_string(hex128(a.cook_key)) << ",\n";
        o << "      \"source_hash\": " << json_string(hex128(a.source_hash)) << ",\n";
        o << "      \"cache_hit\": " << (a.cache_hit ? "true" : "false") << ",\n";
        o << "      \"cook_time_ms\": " << a.cook_time_ms << ",\n";
        o << "      \"deps\": [";
        for (std::size_t d = 0; d < a.deps.size(); ++d) {
            o << json_string(a.deps[d]);
            if (d + 1 < a.deps.size()) o << ", ";
        }
        o << "]\n";
        o << "    }";
        if (i + 1 < assets.size()) o << ",";
        o << "\n";
    }
    o << "  ]\n";
    o << "}\n";
    return o.str();
}

bool CookManifest::write_file(const std::string& path) const {
    std::ofstream f(path, std::ios::out | std::ios::trunc);
    if (!f) return false;
    f << to_json();
    return f.good();
}

} // namespace gw::assets::cook
