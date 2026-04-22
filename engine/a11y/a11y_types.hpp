#pragma once
// engine/a11y/a11y_types.hpp — ADR-0071, 0072.

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gw::a11y {

enum class ColorMode : std::uint8_t {
    None         = 0,
    Protanopia   = 1,
    Deuteranopia = 2,
    Tritanopia   = 3,
    HighContrast = 4,
};

[[nodiscard]] constexpr std::string_view to_string(ColorMode m) noexcept {
    switch (m) {
        case ColorMode::None:         return "none";
        case ColorMode::Protanopia:   return "protan";
        case ColorMode::Deuteranopia: return "deutan";
        case ColorMode::Tritanopia:   return "tritan";
        case ColorMode::HighContrast: return "hc";
    }
    return "unknown";
}

[[nodiscard]] ColorMode parse_color_mode(std::string_view s) noexcept;

struct ColorMatrix3 {
    // Row-major 3x3 matrix applied as out = M * in (sRGB linear space).
    std::array<float, 9> m{1,0,0, 0,1,0, 0,0,1};
};

enum class Role : std::uint8_t {
    Unknown = 0, Window, Button, CheckBox, Slider, Label, ListItem,
    Menu, MenuItem, ComboBox, Tab, TabList, ProgressBar, Image, Graphic, Application,
};

[[nodiscard]] constexpr std::string_view to_string(Role r) noexcept {
    switch (r) {
        case Role::Unknown:    return "unknown";
        case Role::Window:     return "window";
        case Role::Button:     return "button";
        case Role::CheckBox:   return "checkbox";
        case Role::Slider:     return "slider";
        case Role::Label:      return "label";
        case Role::ListItem:   return "list_item";
        case Role::Menu:       return "menu";
        case Role::MenuItem:   return "menu_item";
        case Role::ComboBox:   return "combobox";
        case Role::Tab:        return "tab";
        case Role::TabList:    return "tab_list";
        case Role::ProgressBar:return "progress_bar";
        case Role::Image:      return "image";
        case Role::Graphic:    return "graphic";
        case Role::Application:return "application";
    }
    return "unknown";
}

struct AccessibleNode {
    std::uint64_t node_id{0};
    Role          role{Role::Unknown};
    std::string   name_utf8{};
    std::string   description_utf8{};
    float         x{0.0f}, y{0.0f}, w{0.0f}, h{0.0f};
    bool          focused{false};
    bool          enabled{true};
    std::uint64_t parent_id{0};
    std::vector<std::uint64_t> children{};
};

enum class Politeness : std::uint8_t { Off = 0, Polite = 1, Assertive = 2 };

[[nodiscard]] constexpr std::string_view to_string(Politeness p) noexcept {
    switch (p) {
        case Politeness::Off:       return "off";
        case Politeness::Polite:    return "polite";
        case Politeness::Assertive: return "assertive";
    }
    return "off";
}

} // namespace gw::a11y
