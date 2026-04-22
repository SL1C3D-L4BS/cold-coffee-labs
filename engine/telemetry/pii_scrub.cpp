// engine/telemetry/pii_scrub.cpp

#include "engine/telemetry/pii_scrub.hpp"

#include <cctype>
#include <string>

namespace gw::telemetry {

std::string scrub_pii(std::string_view in, bool scrub_paths) {
    std::string out;
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (scrub_paths && (in[i] == '/' || in[i] == '\\')) {
            out += "[PATH]";
            while (i + 1 < in.size() && (in[i + 1] == '/' || in[i + 1] == '\\' || in[i + 1] == ':'
                   || std::isalnum(static_cast<unsigned char>(in[i + 1])) || in[i + 1] == '.'
                   || in[i + 1] == '_' || in[i + 1] == '-')) {
                ++i;
            }
            continue;
        }
        if (in[i] == '@') {
            out += "[EMAIL]";
            continue;
        }
        out += in[i];
    }
    return out;
}

} // namespace gw::telemetry
