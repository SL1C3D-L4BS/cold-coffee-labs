#pragma once
// engine/physics/replay.hpp — Phase 12 Wave 12E (ADR-0037).
//
// .gwreplay v1 record / playback. Each frame's record is one line: the
// physics step seed, optional character input, and the post-step hash. A
// player re-executes the same fixed-step sequence and asserts hash parity.

#include "engine/physics/character_controller.hpp"
#include "engine/physics/physics_types.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::physics {

struct ReplayFrame {
    std::uint32_t frame_index{0};
    std::uint64_t rng_seed{0};
    std::uint64_t hash{0};
    glm::vec3     character_move_velocity{0.0f};
    bool          character_jump{false};
    bool          character_crouch{false};
};

struct ReplayHeader {
    std::uint32_t version{1};
    std::string   build_id{};          // git short SHA + preset name
    std::string   platform{};          // "win-x64" / "linux-x64" / "null"
    std::string   jolt_version{};      // e.g. "v5.5.0" or "null"
    std::uint32_t fps_hz{60};
    std::uint32_t solver_vel_iters{10};
    std::uint32_t solver_pos_iters{2};
    float         gravity_y{-9.81f};
    float         fixed_dt{1.0f / 60.0f};
    std::uint64_t world_seed{0};
    std::uint32_t hash_every_n{1};
    std::uint64_t hash_options_fingerprint{0};
};

class ReplayRecorder {
public:
    ReplayRecorder() noexcept = default;

    void set_header(ReplayHeader header) { header_ = std::move(header); }

    void record(const ReplayFrame& frame) { frames_.push_back(frame); }

    [[nodiscard]] const ReplayHeader& header() const noexcept { return header_; }
    [[nodiscard]] std::span<const ReplayFrame> frames() const noexcept { return frames_; }
    [[nodiscard]] std::size_t frame_count() const noexcept { return frames_.size(); }

    [[nodiscard]] bool save(const std::filesystem::path& path) const;
    [[nodiscard]] std::string to_string() const;

    void clear() noexcept { frames_.clear(); }

private:
    ReplayHeader             header_{};
    std::vector<ReplayFrame> frames_{};
};

class ReplayPlayer {
public:
    [[nodiscard]] bool load(const std::filesystem::path& path);
    [[nodiscard]] bool load_from_string(std::string_view text);

    [[nodiscard]] const ReplayHeader& header() const noexcept { return header_; }
    [[nodiscard]] std::span<const ReplayFrame> frames() const noexcept { return frames_; }

    // Returns the expected hash for a given frame, if one was recorded.
    [[nodiscard]] std::optional<std::uint64_t> expected_hash(std::uint32_t frame_index) const noexcept;

private:
    ReplayHeader             header_{};
    std::vector<ReplayFrame> frames_{};
};

} // namespace gw::physics
