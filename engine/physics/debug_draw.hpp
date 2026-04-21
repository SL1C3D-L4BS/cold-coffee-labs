#pragma once
// engine/physics/debug_draw.hpp — Phase 12 Wave 12D (ADR-0036).
//
// Flag mask + sink interface for the physics debug surface. The
// `physics_debug_pass` frame-graph node owns a `DebugDrawSink` that the
// backend fills each frame when `physics.debug.enabled` is true.

#include <cstdint>
#include <span>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace gw::physics {

enum class DebugDrawFlag : std::uint32_t {
    None        = 0,
    Bodies      = 1u << 0,
    Contacts    = 1u << 1,
    BroadPhase  = 1u << 2,
    Character   = 1u << 3,
    Constraints = 1u << 4,
    Queries     = 1u << 5,
    All         = 0x3Fu,
};

[[nodiscard]] constexpr std::uint32_t debug_flag_bit(DebugDrawFlag f) noexcept {
    return static_cast<std::uint32_t>(f);
}

struct DebugLine {
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
    glm::vec4 rgba{1.0f, 1.0f, 1.0f, 1.0f};
};

struct DebugTri {
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
    glm::vec3 c{0.0f};
    glm::vec4 rgba{1.0f, 1.0f, 1.0f, 1.0f};
};

// Append-only sink. `physics_debug_pass` constructs this, hands it to
// `PhysicsWorld::fill_debug_draw`, and then feeds buffers to the line /
// triangle pipeline shared with the Phase-7 editor gizmos.
class DebugDrawSink {
public:
    void clear() noexcept {
        lines_.clear();
        tris_.clear();
        flag_mask_ = 0;
    }

    void push_line(const DebugLine& l) { lines_.push_back(l); }
    void push_tri(const DebugTri& t)   { tris_.push_back(t); }

    [[nodiscard]] std::span<const DebugLine> lines() const noexcept { return lines_; }
    [[nodiscard]] std::span<const DebugTri>  tris()  const noexcept { return tris_; }

    void set_flag_mask(std::uint32_t mask) noexcept { flag_mask_ = mask; }
    [[nodiscard]] std::uint32_t flag_mask() const noexcept { return flag_mask_; }

    void set_max_lines(std::uint32_t max_lines) noexcept { max_lines_ = max_lines; }
    [[nodiscard]] bool can_push_line() const noexcept {
        return max_lines_ == 0 || lines_.size() < max_lines_;
    }

private:
    std::vector<DebugLine> lines_{};
    std::vector<DebugTri>  tris_{};
    std::uint32_t          flag_mask_{0};
    std::uint32_t          max_lines_{32768};
};

} // namespace gw::physics
