#pragma once
// engine/ui/ui_types.hpp — shared UI types (ADR-0026).

#include <cstdint>
#include <string>
#include <string_view>

namespace gw::ui {

// Unique identifier for a loaded UI document (.rml). Allocated by UIService.
struct DocumentHandle {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const DocumentHandle&) const noexcept = default;
};

// Unique identifier for a loaded font face.
struct FontFaceId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const FontFaceId&) const noexcept = default;
};

// 32-bit RGBA colour.  Matches RmlUi::Colourb byte order.
struct Color4 {
    std::uint8_t r{0}, g{0}, b{0}, a{255};
    constexpr bool operator==(const Color4&) const noexcept = default;
};

// 2-D integer box (pixels). Top-left origin.
struct RectI {
    std::int32_t x{0}, y{0};
    std::int32_t w{0}, h{0};
};

// 2-D float point.
struct PointF {
    float x{0.0f};
    float y{0.0f};
};

// A localized string resolved through LocaleBridge. Identified by a stable
// 32-bit hash of the canonical key ("hud.health.label" etc.). Phase 16 will
// replace the naive hash with ICU-backed bundle lookup.
struct StringId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
};

// UI pointer capture state — returned by pointer/forward-input APIs so the
// sandbox can decide whether to forward the event to gameplay.
enum class InputCapture : std::uint8_t {
    None = 0,     // UI did not consume the event; forward to gameplay
    Hover,        // a UI element is under the cursor (non-exclusive)
    Captured,     // UI consumed the event; do not forward
};

// Cursor shapes we expose to the OS via Rml::SystemInterface::SetMouseCursor.
enum class Cursor : std::uint8_t {
    Default = 0,
    Pointer,
    Move,
    Cross,
    Text,
    Wait,
    NotAllowed,
    ResizeNS,
    ResizeEW,
};

} // namespace gw::ui
