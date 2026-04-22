#pragma once
// engine/vfx/decals/decal_renderer.hpp — ADR-0078 Wave 17E.

#include "engine/vfx/decals/decal_volume.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gw::vfx::decals {

struct DecalBin {
    std::vector<DecalId> ids;
};

// Builds a coarse tile → decal list using project_to_tile(). Deterministic.
class DecalBinner {
public:
    DecalBinner(std::uint32_t width, std::uint32_t height, std::uint32_t tile_size);

    void clear() noexcept { bins_.clear(); }
    void insert(DecalId id, const DecalVolume& v);

    [[nodiscard]] const DecalBin* find(std::uint32_t tile_x, std::uint32_t tile_y) const noexcept;
    [[nodiscard]] std::size_t     bin_count() const noexcept { return bins_.size(); }

    [[nodiscard]] std::uint32_t tile_size() const noexcept { return tile_size_; }
    [[nodiscard]] std::uint32_t width()     const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height()    const noexcept { return height_; }

private:
    [[nodiscard]] std::uint64_t key_for(std::uint32_t x, std::uint32_t y) const noexcept {
        return (static_cast<std::uint64_t>(x) << 32) | y;
    }

    std::uint32_t width_{0};
    std::uint32_t height_{0};
    std::uint32_t tile_size_{16};
    std::unordered_map<std::uint64_t, DecalBin> bins_{};
};

} // namespace gw::vfx::decals
