// engine/ui/glyph_atlas.cpp — ADR-0028 Wave 11D.
//
// Simple shelf-packer: walk left-to-right along the current row; open a
// new row when we exceed the page width. Open a new page when the row
// can't fit vertically either. No rotation, no maximal-rects — this is
// deliberately straightforward because Phase 11 UI budgets are small
// (HUD + menus + console). Phase 16 will swap in a maximal-rects packer
// when localized text pushes the atlas to pathological sizes.

#include "engine/ui/glyph_atlas.hpp"

#include <cstring>

namespace gw::ui {

AtlasPage& GlyphAtlas::current_page_(bool color) {
    for (auto& p : pages_) {
        if (p.color == color) return p;
    }
    AtlasPage p;
    p.color = color;
    p.width = page_size_;
    p.height = page_size_;
    const std::size_t bpp = color ? 4u : 1u;
    p.pixels.assign(static_cast<std::size_t>(page_size_) * page_size_ * bpp, 0u);
    pages_.push_back(std::move(p));
    return pages_.back();
}

GlyphRect GlyphAtlas::bake(const GlyphKey& key,
                           std::uint16_t w, std::uint16_t h,
                           std::span<const std::uint8_t> pixel_payload,
                           std::int16_t bearing_x, std::int16_t bearing_y,
                           std::int32_t advance_x_26_6) {
    if (w == 0 || h == 0) return GlyphRect{};
    if (w > page_size_ || h > page_size_) return GlyphRect{};  // oversize

    auto& page = current_page_(key.color);

    if (page.cursor_x + w > page.width) {
        page.cursor_x = 0;
        page.cursor_y = static_cast<std::uint16_t>(page.cursor_y + page.row_height);
        page.row_height = 0;
    }
    if (page.cursor_y + h > page.height) {
        // Open a new page of the same color type.
        AtlasPage np;
        np.color = key.color;
        np.width = page_size_;
        np.height = page_size_;
        const std::size_t bpp = key.color ? 4u : 1u;
        np.pixels.assign(static_cast<std::size_t>(page_size_) * page_size_ * bpp, 0u);
        pages_.push_back(std::move(np));
    }

    auto& dst = pages_.back();
    const std::size_t bpp = key.color ? 4u : 1u;

    GlyphRect rect;
    rect.atlas_page = static_cast<std::uint16_t>(pages_.size() - 1);
    rect.x = dst.cursor_x;
    rect.y = dst.cursor_y;
    rect.w = w;
    rect.h = h;
    rect.bearing_x = bearing_x;
    rect.bearing_y = bearing_y;
    rect.advance_x_26_6 = advance_x_26_6;

    if (pixel_payload.size() >= static_cast<std::size_t>(w) * h * bpp) {
        for (std::uint16_t row = 0; row < h; ++row) {
            const std::uint8_t* src = pixel_payload.data() + static_cast<std::size_t>(row) * w * bpp;
            std::uint8_t* dp = dst.pixels.data()
                + (static_cast<std::size_t>(rect.y + row) * dst.width + rect.x) * bpp;
            std::memcpy(dp, src, static_cast<std::size_t>(w) * bpp);
        }
    }

    dst.cursor_x = static_cast<std::uint16_t>(dst.cursor_x + w);
    if (h > dst.row_height) dst.row_height = h;

    entries_.push_back(Entry{key, rect});
    return rect;
}

const GlyphRect* GlyphAtlas::lookup(const GlyphKey& key) const noexcept {
    for (const auto& e : entries_) {
        if (e.key == key) return &e.rect;
    }
    return nullptr;
}

} // namespace gw::ui
