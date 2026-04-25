// engine/replay/gameplay_replay_validate.cpp
#include "engine/replay/gameplay_replay_validate.hpp"

#include <cctype>
#include <charconv>
#include <vector>

namespace gw::replay {
namespace {

void skip_ws(std::string_view& s) noexcept {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) {
        s.remove_prefix(1);
    }
}

bool parse_i32(std::string_view& s, int& out) noexcept {
    skip_ws(s);
    if (s.empty()) {
        return false;
    }
    const char* b   = s.data();
    const char* e   = s.data() + s.size();
    int         v   = 0;
    const auto  res = std::from_chars(b, e, v);
    if (res.ec != std::errc() || res.ptr == b) {
        return false;
    }
    out = v;
    s   = s.substr(static_cast<std::size_t>(res.ptr - b));
    return true;
}

bool read_version_value(const std::string_view json, int& version) noexcept {
    constexpr std::string_view kKey = "\"version\"";
    for (std::size_t i = 0; i + kKey.size() <= json.size(); ++i) {
        if (json.compare(i, kKey.size(), kKey) != 0) {
            continue;
        }
        std::string_view tail = json.substr(i + kKey.size());
        skip_ws(tail);
        if (tail.empty() || tail.front() != ':') {
            continue;
        }
        tail.remove_prefix(1);
        skip_ws(tail);
        return parse_i32(tail, version);
    }
    return false;
}

} // namespace

bool validate_gameplay_replay_json_loose(const std::string_view json) noexcept {
    if (json.find('{') == std::string_view::npos) {
        return false;
    }
    int ver = 0;
    if (!read_version_value(json, ver) || ver != 1) {
        return false;
    }
    std::vector<int> ticks;
    ticks.reserve(32U);
    constexpr std::string_view kTick = "\"tick\"";
    for (std::size_t i = 0; i < json.size();) {
        const auto pos = json.find(kTick, i);
        if (pos == std::string_view::npos) {
            break;
        }
        std::string_view tail = json.substr(pos + kTick.size());
        skip_ws(tail);
        if (tail.empty() || tail.front() != ':') {
            i = pos + 1U;
            continue;
        }
        tail.remove_prefix(1);
        skip_ws(tail);
        int t = 0;
        if (!parse_i32(tail, t)) {
            i = pos + 1U;
            continue;
        }
        ticks.push_back(t);
        i = pos + kTick.size();
    }
    if (ticks.size() < 2U) {
        return true;
    }
    for (std::size_t k = 1; k < ticks.size(); ++k) {
        if (ticks[k] <= ticks[k - 1U]) {
            return false;
        }
    }
    return true;
}

} // namespace gw::replay
