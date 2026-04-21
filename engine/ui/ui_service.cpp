// engine/ui/ui_service.cpp — ADR-0026 Wave 11D.
//
// Null implementation used when GW_UI_RMLUI is not defined. Maintains the
// full API contract (documents load/unload, input is accepted, stats tick)
// without pulling the RmlUi dependency. The RmlUi-enabled translation unit
// replaces the bodies of `load_document`, `update`, `render`, the
// `forward_*` family, and the constructor's Rml::Initialise call.

#include "engine/ui/ui_service.hpp"
#include "engine/core/config/cvar_registry.hpp"

#include <chrono>

namespace gw::ui {

UIService::UIService(UIConfig cfg,
                     config::CVarRegistry& cvars,
                     FontLibrary& fonts,
                     GlyphAtlas&  atlas,
                     TextShaper&  shaper,
                     LocaleBridge& locale,
                     events::EventBus<events::UILayoutComplete>* layout_bus,
                     events::EventBus<events::WindowResized>*    resize_bus)
    : cfg_(cfg), cvars_(cvars), fonts_(fonts), atlas_(atlas),
      shaper_(shaper), locale_(locale),
      layout_bus_(layout_bus), resize_bus_(resize_bus) {
    sync_cvars_();
}

UIService::~UIService() = default;

bool UIService::rml_enabled() const noexcept {
#if defined(GW_UI_RMLUI) && GW_UI_RMLUI
    return true;
#else
    return false;
#endif
}

void UIService::sync_cvars_() {
    text_scale_     = cvars_.get_f32_or("ui.text.scale", 1.0f);
    contrast_boost_ = cvars_.get_bool_or("ui.contrast.boost", false);
    reduce_motion_  = cvars_.get_bool_or("ui.reduce_motion", false);
}

bool UIService::load_document(std::string_view rml_path, DocumentHandle& out_handle) {
    if (rml_path.empty()) return false;
    Document d;
    d.handle = DocumentHandle{next_document_id_++};
    d.path.assign(rml_path.data(), rml_path.size());
    d.visible = true;
    d.native = nullptr;
    documents_.push_back(std::move(d));
    out_handle = documents_.back().handle;
    return true;
}

bool UIService::unload_document(DocumentHandle handle) {
    for (auto it = documents_.begin(); it != documents_.end(); ++it) {
        if (it->handle == handle) {
            documents_.erase(it);
            return true;
        }
    }
    return false;
}

void UIService::update(double delta_seconds) {
    (void)delta_seconds;
    const auto t0 = std::chrono::steady_clock::now();

    // Re-read theming CVars once per frame (cheap — three vector walks).
    sync_cvars_();

    stats_.documents_updated = static_cast<std::uint32_t>(documents_.size());
    ++stats_.frame_index;

    const auto t1 = std::chrono::steady_clock::now();
    stats_.ns_update =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
}

void UIService::render() {
    const auto t0 = std::chrono::steady_clock::now();

    // Null backend: no real render list — publish a UILayoutComplete so
    // downstream systems can prove the seam works.
    if (layout_bus_) {
        events::UILayoutComplete ev;
        ev.frame_index       = stats_.frame_index;
        ev.elements_rendered = stats_.documents_updated;   // proxy metric
        layout_bus_->publish(ev);
    }
    stats_.elements_rendered = stats_.documents_updated;

    const auto t1 = std::chrono::steady_clock::now();
    stats_.ns_render =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
}

// -------- Input forwarding stubs --------

void UIService::forward_mouse_move(float, float) {}

InputCapture UIService::forward_mouse_button(std::uint32_t, bool) {
    return InputCapture::None;
}

InputCapture UIService::forward_key(std::uint32_t, bool, std::uint32_t) {
    return InputCapture::None;
}

InputCapture UIService::forward_text(std::string_view) {
    return InputCapture::None;
}

InputCapture UIService::forward_scroll(float, float) {
    return InputCapture::None;
}

InputCapture UIService::forward_gamepad_nav(std::int32_t, std::int32_t, bool) {
    return InputCapture::None;
}

// -------- Surface --------

void UIService::set_viewport(std::uint32_t w, std::uint32_t h, float dpi) {
    cfg_.width_px   = w;
    cfg_.height_px  = h;
    cfg_.dpi_scale  = dpi;
    if (resize_bus_) {
        events::WindowResized ev;
        ev.width     = w;
        ev.height    = h;
        ev.dpi_scale = dpi;
        resize_bus_->publish(ev);
    }
}

// -------- Theming / a11y --------

void UIService::set_text_scale(float scale) noexcept {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 4.0f) scale = 4.0f;
    text_scale_ = scale;
}

void UIService::set_contrast_boost(bool on) noexcept { contrast_boost_ = on; }
void UIService::set_reduce_motion(bool on)  noexcept { reduce_motion_  = on; }

} // namespace gw::ui
