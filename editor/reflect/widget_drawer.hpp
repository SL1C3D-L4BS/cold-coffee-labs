#pragma once
// editor/reflect/widget_drawer.hpp
// Per-type ImGui widget dispatch for the inspector panel.
// Spec ref: Phase 7 §3 + §10.

#include "reflect.hpp"
#include <cstdint>
#include <functional>

namespace gw::editor {

// DrawFn: given FieldInfo + raw byte pointer to the field, render widgets and
// return true if the value was modified this frame.
using DrawFn = std::function<bool(const reflect::FieldInfo& fi, void* ptr)>;

// ---------------------------------------------------------------------------
// WidgetDrawer
// Dispatch table sorted by type_id (FNV1a) → O(log n) lookup via lower_bound.
// Built once at startup; the editor calls draw_field() each frame.
// ---------------------------------------------------------------------------
class WidgetDrawer {
public:
    WidgetDrawer();

    // Register a custom draw function for a type_id.
    void register_type(uint32_t type_id, DrawFn fn);

    // Dispatch: returns true if the value was modified.
    [[nodiscard]] bool draw_field(const reflect::FieldInfo& fi,
                                  void*                     ptr) const;

    // Draw all fields of a reflected component.
    template<typename T>
    [[nodiscard]] bool draw_component(T& comp) const {
        const reflect::TypeInfo& ti = describe(static_cast<T*>(nullptr));
        bool changed = false;
        for (const auto& f : ti.fields) {
            void* ptr = reinterpret_cast<uint8_t*>(&comp) + f.offset;
            changed |= draw_field(f, ptr);
        }
        return changed;
    }

private:
    struct Entry {
        uint32_t type_id;
        DrawFn   fn;
        bool operator<(const Entry& o) const noexcept { return type_id < o.type_id; }
        bool operator<(uint32_t id)    const noexcept { return type_id < id; }
    };

    std::vector<Entry> table_;   // sorted by type_id; built in ctor, then frozen
    bool               frozen_ = false;

    void sort_and_freeze();
    void register_builtin_types();
};

// Global singleton (created by EditorApplication, valid for editor lifetime).
WidgetDrawer& get_widget_drawer() noexcept;

}  // namespace gw::editor
