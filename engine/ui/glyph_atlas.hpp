#pragma once
// engine/ui/glyph_atlas.hpp — SDF + COLRv1 glyph atlas (ADR-0028).

#include "engine/ui/ui_types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gw::ui {

struct GlyphKey {
    FontFaceId    face{};
    std::uint32_t glyph_index{0};  // FT glyph index (post fallback)
    std::uint32_t pixel_size{16};
    bool          color{false};    // true for COLRv1 emoji
    constexpr bool operator==(const GlyphKey&) const noexcept = default;
};

struct GlyphRect {
    std::uint16_t atlas_page{0};
    std::uint16_t x{0}, y{0};
    std::uint16_t w{0}, h{0};
    std::int16_t  bearing_x{0};
    std::int16_t  bearing_y{0};
    std::int32_t  advance_x_26_6{0};  // FT 26.6 fixed
};

struct AtlasPage {
    std::uint16_t width{0};
    std::uint16_t height{0};
    std::uint16_t cursor_x{0};
    std::uint16_t cursor_y{0};
    std::uint16_t row_height{0};
    bool          color{false};       // true for BGRA COLRv1 page
    std::vector<std::uint8_t> pixels; // R8 (greyscale) or BGRA8 (color)
};

class GlyphAtlas {
public:
    // Page size in pixels. Phase 11 uses 2048×2048.
    explicit GlyphAtlas(std::uint16_t page_size = 2048) : page_size_(page_size) {}
    ~GlyphAtlas() = default;
    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;

    // Allocate a rectangle for a new glyph. `pixel_payload` is either
    // R8 (len = w*h) or BGRA8 (len = w*h*4). Returns a copy of the
    // resulting GlyphRect. On failure (page full / oversize glyph) the
    // returned rect has w==0.
    GlyphRect bake(const GlyphKey& key,
                   std::uint16_t w, std::uint16_t h,
                   std::span<const std::uint8_t> pixel_payload,
                   std::int16_t bearing_x, std::int16_t bearing_y,
                   std::int32_t advance_x_26_6);

    // Look up a previously baked glyph.
    [[nodiscard]] const GlyphRect* lookup(const GlyphKey& key) const noexcept;

    [[nodiscard]] std::size_t glyph_count()    const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t page_count()     const noexcept { return pages_.size(); }
    [[nodiscard]] const AtlasPage& page(std::size_t i) const noexcept { return pages_[i]; }
    [[nodiscard]] std::uint16_t    page_size() const noexcept { return page_size_; }

private:
    struct Entry {
        GlyphKey  key{};
        GlyphRect rect{};
    };

    AtlasPage& current_page_(bool color);

    std::uint16_t          page_size_;
    std::vector<AtlasPage> pages_{};
    std::vector<Entry>     entries_{};
};

} // namespace gw::ui
