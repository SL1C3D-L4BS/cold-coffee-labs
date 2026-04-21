// engine/ui/font_library.cpp — ADR-0028 Wave 11D.
//
// Null-backend implementation: metadata only, no FT_Library. The gated
// FreeType build will swap the `.cpp` out via the GW_UI_FREETYPE compile
// definition. That swap lives under cmake/dependencies.cmake.

#include "engine/ui/font_library.hpp"

namespace gw::ui {

FontFaceId FontLibrary::register_face(const FontFace& face) {
    faces_.push_back(face);
    return FontFaceId{static_cast<std::uint32_t>(faces_.size())};
}

bool FontLibrary::load_face(FontFaceId id, std::span<const std::byte> file_bytes) {
    if (!id.valid() || id.value > faces_.size()) return false;
    bytes_loaded_ += file_bytes.size();
    return true;
}

void FontLibrary::set_fallback_chain(std::string_view chain_name,
                                     std::span<const FontFaceId> order) {
    for (auto& c : chains_) {
        if (c.name == chain_name) {
            c.order.assign(order.begin(), order.end());
            return;
        }
    }
    FallbackChain c;
    c.name.assign(chain_name.data(), chain_name.size());
    c.order.assign(order.begin(), order.end());
    chains_.push_back(std::move(c));
}

const FontFace* FontLibrary::face(FontFaceId id) const noexcept {
    if (!id.valid() || id.value > faces_.size()) return nullptr;
    return &faces_[id.value - 1];
}

const FallbackChain* FontLibrary::chain(std::string_view name) const noexcept {
    for (const auto& c : chains_) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

} // namespace gw::ui
