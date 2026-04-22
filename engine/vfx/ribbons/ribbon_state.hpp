#pragma once
// engine/vfx/ribbons/ribbon_state.hpp — ADR-0078 Wave 17E.

#include <array>
#include <cstdint>
#include <vector>

namespace gw::vfx::ribbons {

struct RibbonVertex {
    std::array<float, 3> position{0, 0, 0};
    float                age{0.0f};
    std::array<float, 2> uv{0.0f, 0.0f};
    float                width{0.1f};
    float                _pad{0.0f};
};
static_assert(sizeof(RibbonVertex) == 32, "RibbonVertex is 32 B frozen (ADR-0078 §4)");

struct RibbonId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const RibbonId&) const noexcept = default;
};

struct RibbonDesc {
    std::uint32_t max_length{64};        // max vertex-pair count (head..tail)
    float         node_period_s{1.0f / 60.0f}; // Nyquist pace — append at least this often
    float         lifetime_s{1.0f};
    float         width{0.1f};
};

class RibbonState {
public:
    explicit RibbonState(RibbonDesc d) : desc_{d} { verts_.reserve(d.max_length); }

    [[nodiscard]] const RibbonDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] std::size_t       size() const noexcept { return verts_.size(); }
    [[nodiscard]] const std::vector<RibbonVertex>& vertices() const noexcept { return verts_; }

    // Appends a new node; oldest is dropped when ring is full.
    void append(const std::array<float, 3>& position, float dt_seconds);

    // Advances age; removes nodes older than lifetime_s.
    void advance(float dt_seconds);

    void clear() noexcept { verts_.clear(); acc_ = 0.0f; }

    // Helpers for unit tests / GPU upload.
    [[nodiscard]] float accumulator() const noexcept { return acc_; }

private:
    RibbonDesc               desc_{};
    std::vector<RibbonVertex> verts_{};
    float                     acc_{0.0f};
};

} // namespace gw::vfx::ribbons
