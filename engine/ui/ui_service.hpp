#pragma once
// engine/ui/ui_service.hpp — RmlUi context owner + input forwarding (ADR-0026).
//
// Phase 11 ships a *null* UIService when GW_UI_RMLUI is OFF: every method is
// a no-op that exposes the right signatures so the runtime can compile.
// When GW_UI_RMLUI=1 the service owns an Rml::Context, wires the custom
// SystemInterface / FileInterface / RenderInterface, and dispatches input
// events it receives through the input service's per-frame drain.
//
// Threading: main thread only. The render pass queries `frame_geometry()`
// from the render thread after UIService has populated it on main thread.

#include "engine/ui/font_library.hpp"
#include "engine/ui/glyph_atlas.hpp"
#include "engine/ui/locale_bridge.hpp"
#include "engine/ui/text_shaper.hpp"
#include "engine/ui/ui_types.hpp"

#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gw::config { class CVarRegistry; }

namespace gw::ui {

struct UIConfig {
    std::uint32_t width_px{1920};
    std::uint32_t height_px{1080};
    float         dpi_scale{1.0f};
    // Font chain that the sandbox HUD uses. Falls back through the chain
    // when a glyph is missing in the primary face.
    std::string   default_font_chain{"ui.default"};
    // Debug: enable the RmlUi debugger overlay on F1.
    bool          enable_debugger{false};
};

struct UIFrameStats {
    std::uint64_t frame_index{0};
    std::uint32_t documents_updated{0};
    std::uint32_t elements_rendered{0};
    std::uint32_t glyphs_shaped{0};
    std::uint64_t ns_update{0};
    std::uint64_t ns_render{0};
};

class UIService {
public:
    UIService(UIConfig cfg,
              config::CVarRegistry& cvars,
              FontLibrary& fonts,
              GlyphAtlas&  atlas,
              TextShaper&  shaper,
              LocaleBridge& locale,
              events::EventBus<events::UILayoutComplete>* layout_bus = nullptr,
              events::EventBus<events::WindowResized>*    resize_bus = nullptr);
    ~UIService();
    UIService(const UIService&) = delete;
    UIService& operator=(const UIService&) = delete;

    // -------- Lifecycle -------- (RAII; no two-phase init.)
    [[nodiscard]] bool load_document(std::string_view rml_path, DocumentHandle& out_handle);
    [[nodiscard]] bool unload_document(DocumentHandle handle);

    // Call once per frame *before* render. Updates layout, processes
    // animations, drains pending events.
    void update(double delta_seconds);

    // Call once per frame *after* update to compile the render list. The
    // render backend consumes this in the ui_pass frame-graph node. Safe
    // to call from the render thread once a frame has been built.
    void render();

    // -------- Input forwarding --------
    void forward_mouse_move(float x_px, float y_px);
    InputCapture forward_mouse_button(std::uint32_t button, bool down);
    InputCapture forward_key(std::uint32_t keycode, bool down, std::uint32_t mods);
    InputCapture forward_text(std::string_view utf8);
    InputCapture forward_scroll(float dx, float dy);
    InputCapture forward_gamepad_nav(std::int32_t dx, std::int32_t dy, bool activate);

    // -------- Surface --------
    void set_viewport(std::uint32_t width_px, std::uint32_t height_px, float dpi_scale);
    [[nodiscard]] UIConfig viewport() const noexcept { return cfg_; }

    // -------- Theming / a11y --------
    void set_text_scale(float scale) noexcept;
    void set_contrast_boost(bool on) noexcept;
    void set_reduce_motion(bool on) noexcept;
    [[nodiscard]] float  text_scale()     const noexcept { return text_scale_; }
    [[nodiscard]] bool   contrast_boost() const noexcept { return contrast_boost_; }
    [[nodiscard]] bool   reduce_motion()  const noexcept { return reduce_motion_; }

    // -------- Inspection --------
    [[nodiscard]] const UIFrameStats& stats() const noexcept { return stats_; }
    [[nodiscard]] std::size_t document_count() const noexcept { return documents_.size(); }
    [[nodiscard]] bool rml_enabled() const noexcept;  // true when compiled with GW_UI_RMLUI

private:
    struct Document {
        DocumentHandle  handle{};
        std::string     path{};
        bool            visible{true};
        // GW_UI_RMLUI build stores Rml::ElementDocument* here via opaque ptr.
        void*           native{nullptr};
    };

    // Applies the ui.text.scale / ui.contrast.boost / ui.reduce_motion
    // CVars to local caches at construction + on ConfigCVarChanged.
    void sync_cvars_();

    UIConfig                                          cfg_{};
    config::CVarRegistry&                             cvars_;
    FontLibrary&                                      fonts_;
    GlyphAtlas&                                       atlas_;
    TextShaper&                                       shaper_;
    LocaleBridge&                                     locale_;
    events::EventBus<events::UILayoutComplete>*       layout_bus_{nullptr};
    events::EventBus<events::WindowResized>*          resize_bus_{nullptr};

    std::vector<Document>    documents_{};
    UIFrameStats             stats_{};

    // Theming caches (mirrored from CVars).
    float  text_scale_{1.0f};
    bool   contrast_boost_{false};
    bool   reduce_motion_{false};

    std::uint32_t next_document_id_{1};

    // GW_UI_RMLUI build stashes an Rml::Context* here via opaque ptr so we
    // don't pull Rml headers into engine/ui's public surface.
    void* rml_context_{nullptr};
};

} // namespace gw::ui
