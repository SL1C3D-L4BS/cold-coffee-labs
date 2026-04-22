# ADR 0072 — Screen-reader bridge (UIA / AT-SPI / NSAccessibility)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16E)
- **Pins:** AccessKit-C **0.14.0** (2026-Q1).
- **Companions:** 0071 (a11y modes), 0073.

## 1. Decision

Ship a cross-platform screen-reader bridge via **AccessKit-C 0.14.0**, a
Rust-core C-ABI that speaks MS UI Automation (Windows), AT-SPI 2 (Linux),
and NSAccessibility (macOS, out of scope but kept on the table). The
bridge is behind `GW_ENABLE_ACCESSKIT`; the null build ships an
in-memory accessibility tree that passes the selfcheck but does not
talk to any OS API.

## 2. Interface

```cpp
namespace gw::a11y {

enum class Role : std::uint8_t {
    Unknown, Window, Button, CheckBox, Slider, Label, ListItem,
    Menu, MenuItem, ComboBox, Tab, TabList, ProgressBar, Image,
    Graphic, Application,
};

struct AccessibleNode {
    std::uint64_t node_id;         // stable across frames
    Role          role{Role::Unknown};
    std::string   name_utf8;       // already localized
    std::string   description_utf8;
    float         x{0}, y{0}, w{0}, h{0};  // virtual-pixel AABB
    bool          focused{false};
    bool          enabled{true};
    std::uint64_t parent_id{0};
    std::vector<std::uint64_t> children;
};

class IScreenReader {
public:
    virtual ~IScreenReader() = default;

    virtual void update_tree(std::span<const AccessibleNode> nodes) = 0;
    virtual void announce_focus(std::uint64_t node_id) = 0;
    virtual void announce_text(std::string_view utf8,
                               std::string_view politeness) = 0;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
};

std::unique_ptr<IScreenReader> make_null_screen_reader();
std::unique_ptr<IScreenReader> make_accesskit_screen_reader();

} // namespace gw::a11y
```

## 3. Bridging RmlUi

`engine/ui/ui_service` emits an `a11y::TreeDelta` after each layout when
`a11y.screen_reader.enabled`. `IScreenReader::update_tree` consumes it;
focus changes emit `announce_focus(id)` with the visible node name.

## 4. Politeness levels

Matches ARIA live regions: `off`, `polite`, `assertive`. Subtitles +
HUD updates default to `polite`; critical warnings (damage, low-O2)
use `assertive`. `a11y.screen_reader.verbosity` multiplexes: `terse`
skips `polite` announcements entirely.

## 5. Header quarantine

`<accesskit.h>` only in `engine/a11y/impl/accesskit_backend.cpp`. Public
headers stay platform-neutral.

## 6. Selfcheck tick items

- Every interactive element has a role + accessible name.
- Tab order == visual order.
- Focus ring is drawn.
- No `Role::Unknown` in a built scene.

Passing all four → `a11y::SelfCheckReport::screen_reader = Pass`.
