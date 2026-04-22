// engine/vfx/decals/decal_renderer.cpp — ADR-0078 Wave 17E.

#include "engine/vfx/decals/decal_renderer.hpp"

namespace gw::vfx::decals {

DecalBinner::DecalBinner(std::uint32_t width, std::uint32_t height, std::uint32_t tile_size)
    : width_{width}, height_{height}, tile_size_{tile_size == 0 ? 16u : tile_size} {}

void DecalBinner::insert(DecalId id, const DecalVolume& v) {
    const auto t = project_to_tile(v.position, width_, height_, tile_size_);
    bins_[key_for(t.x, t.y)].ids.push_back(id);
}

const DecalBin* DecalBinner::find(std::uint32_t tile_x, std::uint32_t tile_y) const noexcept {
    const auto it = bins_.find(key_for(tile_x, tile_y));
    return it == bins_.end() ? nullptr : &it->second;
}

} // namespace gw::vfx::decals
