#pragma once
// engine/ui/font_library.hpp — FT_Face cache + fallback chain (ADR-0028).
//
// In a FreeType-enabled build (GW_UI_FREETYPE=1) this owns FT_Library +
// FT_Face instances. In a null/gated build it stores metadata only and
// exposes a deterministic API so upstream systems compile and run tests
// without the freetype dependency.

#include "engine/ui/ui_types.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::ui {

struct FontFace {
    std::string   family{};          // "Inter", "Noto Sans CJK SC"
    std::string   style{};           // "Regular", "Bold", "Italic"
    std::string   path{};            // VFS path to .ttf/.otf
    std::uint32_t weight{400};       // CSS weight (100..900)
    bool          has_emoji_color{false};  // COLRv1 glyph table present
    float         line_height{1.2f}; // multiplied by pixel_size
};

struct FallbackChain {
    std::string             name{};
    std::vector<FontFaceId> order{};
};

class FontLibrary {
public:
    FontLibrary() = default;
    ~FontLibrary() = default;
    FontLibrary(const FontLibrary&) = delete;
    FontLibrary& operator=(const FontLibrary&) = delete;

    // Register a font face by metadata. Actual file load is deferred to
    // `load_face`. Returns a handle used to refer to the face.
    FontFaceId register_face(const FontFace& face);

    // Load the actual font file bytes. In the FreeType build this calls
    // FT_New_Memory_Face; in the null build it merely records the size.
    bool load_face(FontFaceId id, std::span<const std::byte> file_bytes);

    // Fallback chain: if a glyph is missing in `primary`, the library walks
    // `chain` in order. Chains are registered per script (Latin / CJK /
    // Emoji). Phase 11 ships a single "ui.default" chain.
    void set_fallback_chain(std::string_view chain_name,
                            std::span<const FontFaceId> order);

    [[nodiscard]] const FontFace* face(FontFaceId id) const noexcept;
    [[nodiscard]] std::size_t face_count() const noexcept { return faces_.size(); }
    [[nodiscard]] const FallbackChain* chain(std::string_view name) const noexcept;

    // Tracked bytes loaded across every face — perf contract observability.
    [[nodiscard]] std::uint64_t bytes_loaded() const noexcept { return bytes_loaded_; }

private:
    std::vector<FontFace>       faces_{};
    std::vector<FallbackChain>  chains_{};
    std::uint64_t               bytes_loaded_{0};
};

} // namespace gw::ui
