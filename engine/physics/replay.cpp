// engine/physics/replay.cpp — Phase 12 Wave 12E (ADR-0037).

#include "engine/physics/replay.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace gw::physics {

namespace {

std::string escape_text(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            out.push_back('_');
        } else {
            out.push_back(c);
        }
    }
    if (out.empty()) out.push_back('-');
    return out;
}

bool parse_header_kv(const std::string& line, ReplayHeader& out) {
    if (line.size() < 2 || line[0] != '#') return true; // comment, ignore
    const auto sp = line.find(' ');
    if (sp == std::string::npos) return true;
    const std::string key = line.substr(1, sp - 1);
    const std::string val = line.substr(sp + 1);
    if (key == "version") {
        out.version = static_cast<std::uint32_t>(std::stoul(val));
    } else if (key == "build_id") {
        out.build_id = val;
    } else if (key == "platform") {
        out.platform = val;
    } else if (key == "jolt_version") {
        out.jolt_version = val;
    } else if (key == "fps_hz") {
        out.fps_hz = static_cast<std::uint32_t>(std::stoul(val));
    } else if (key == "solver_vel_iters") {
        out.solver_vel_iters = static_cast<std::uint32_t>(std::stoul(val));
    } else if (key == "solver_pos_iters") {
        out.solver_pos_iters = static_cast<std::uint32_t>(std::stoul(val));
    } else if (key == "gravity_y") {
        out.gravity_y = std::stof(val);
    } else if (key == "fixed_dt") {
        out.fixed_dt = std::stof(val);
    } else if (key == "world_seed") {
        out.world_seed = std::stoull(val);
    } else if (key == "hash_every_n") {
        out.hash_every_n = static_cast<std::uint32_t>(std::stoul(val));
    } else if (key == "hash_options_fingerprint") {
        out.hash_options_fingerprint = std::stoull(val);
    }
    return true;
}

} // namespace

// -----------------------------------------------------------------------------
// Recorder
// -----------------------------------------------------------------------------
std::string ReplayRecorder::to_string() const {
    std::ostringstream os;
    os << "# GWREPLAY v" << header_.version << "\n";
    os << "#version " << header_.version << "\n";
    os << "#build_id " << escape_text(header_.build_id.empty() ? "unknown" : header_.build_id) << "\n";
    os << "#platform " << escape_text(header_.platform.empty() ? "null" : header_.platform) << "\n";
    os << "#jolt_version " << escape_text(header_.jolt_version.empty() ? "null" : header_.jolt_version) << "\n";
    os << "#fps_hz " << header_.fps_hz << "\n";
    os << "#solver_vel_iters " << header_.solver_vel_iters << "\n";
    os << "#solver_pos_iters " << header_.solver_pos_iters << "\n";
    os << "#gravity_y " << header_.gravity_y << "\n";
    os << "#fixed_dt " << header_.fixed_dt << "\n";
    os << "#world_seed " << header_.world_seed << "\n";
    os << "#hash_every_n " << header_.hash_every_n << "\n";
    os << "#hash_options_fingerprint " << header_.hash_options_fingerprint << "\n";
    os << "#--- frames ---\n";

    for (const auto& f : frames_) {
        os << "f "
           << f.frame_index << ' '
           << f.rng_seed    << ' '
           << f.hash        << ' '
           << f.character_move_velocity.x << ' '
           << f.character_move_velocity.y << ' '
           << f.character_move_velocity.z << ' '
           << (f.character_jump   ? 1 : 0) << ' '
           << (f.character_crouch ? 1 : 0) << '\n';
    }
    return os.str();
}

bool ReplayRecorder::save(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    const std::string s = to_string();
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
    return static_cast<bool>(out);
}

// -----------------------------------------------------------------------------
// Player
// -----------------------------------------------------------------------------
bool ReplayPlayer::load(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream os;
    os << in.rdbuf();
    return load_from_string(os.str());
}

bool ReplayPlayer::load_from_string(std::string_view text) {
    header_ = {};
    frames_.clear();

    std::size_t start = 0;
    while (start < text.size()) {
        auto nl = text.find('\n', start);
        const std::string_view line = (nl == std::string_view::npos)
                                          ? text.substr(start)
                                          : text.substr(start, nl - start);
        if (!line.empty() && line.front() == '#') {
            parse_header_kv(std::string{line}, header_);
        } else if (!line.empty() && line.front() == 'f') {
            ReplayFrame f{};
            std::istringstream iss{std::string{line.substr(1)}};
            int jump = 0, crouch = 0;
            iss >> f.frame_index
                >> f.rng_seed
                >> f.hash
                >> f.character_move_velocity.x
                >> f.character_move_velocity.y
                >> f.character_move_velocity.z
                >> jump
                >> crouch;
            f.character_jump   = jump != 0;
            f.character_crouch = crouch != 0;
            if (!iss.fail()) {
                frames_.push_back(f);
            }
        }
        if (nl == std::string_view::npos) break;
        start = nl + 1;
    }
    return true;
}

std::optional<std::uint64_t> ReplayPlayer::expected_hash(std::uint32_t frame_index) const noexcept {
    for (const auto& f : frames_) {
        if (f.frame_index == frame_index) return f.hash;
    }
    return std::nullopt;
}

} // namespace gw::physics
