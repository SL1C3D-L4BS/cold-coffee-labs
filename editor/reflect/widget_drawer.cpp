// editor/reflect/widget_drawer.cpp
#include "widget_drawer.hpp"
#include "engine/core/type_registry.hpp"

#include <imgui.h>
#include <algorithm>
#include <cstdio>

// GLM for vec2/vec3/vec4/quat widgets.
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace gw::editor {

// kField* flag constants live in gw::core — pull them into this TU.
using gw::core::kFieldReadOnly;
using gw::core::kFieldRange;
using gw::core::kFieldAngle;
using gw::core::kFieldColor;
using gw::core::kFieldHidden;

namespace {

// Stable type IDs for primitive types — computed once at startup.
const uint32_t kTypeFloat   = gw::core::type_id<float>();
const uint32_t kTypeDouble  = gw::core::type_id<double>();
const uint32_t kTypeInt32   = gw::core::type_id<int32_t>();
const uint32_t kTypeUint32  = gw::core::type_id<uint32_t>();
const uint32_t kTypeBool    = gw::core::type_id<bool>();
const uint32_t kTypeVec2    = gw::core::type_id<glm::vec2>();
const uint32_t kTypeVec3    = gw::core::type_id<glm::vec3>();
const uint32_t kTypeVec4    = gw::core::type_id<glm::vec4>();
const uint32_t kTypeQuat    = gw::core::type_id<glm::quat>();

// Widget helpers
bool draw_float(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<float*>(ptr);
    if (fi.flags & kFieldReadOnly) {
        ImGui::Text("%s: %.4f", fi.name, static_cast<double>(v));
        return false;
    }
    if (fi.flags & kFieldRange)
        return ImGui::SliderFloat(fi.name, &v, fi.min_val, fi.max_val);
    if (fi.flags & kFieldAngle) {
        float deg = glm::degrees(v);
        if (ImGui::DragFloat(fi.name, &deg, 0.5f)) {
            v = glm::radians(deg);
            return true;
        }
        return false;
    }
    return ImGui::DragFloat(fi.name, &v, 0.01f);
}

bool draw_double(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<double*>(ptr);
    float fv = static_cast<float>(v);
    if (ImGui::DragFloat(fi.name, &fv, 0.01f)) {
        v = static_cast<double>(fv);
        return true;
    }
    return false;
}

bool draw_int32(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<int32_t*>(ptr);
    if (fi.flags & kFieldReadOnly) {
        ImGui::Text("%s: %d", fi.name, v);
        return false;
    }
    if (fi.flags & kFieldRange)
        return ImGui::SliderInt(fi.name, &v,
            static_cast<int>(fi.min_val), static_cast<int>(fi.max_val));
    return ImGui::DragInt(fi.name, &v);
}

bool draw_uint32(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<uint32_t*>(ptr);
    int iv = static_cast<int>(v);
    if (ImGui::DragInt(fi.name, &iv, 1.f, 0)) {
        v = static_cast<uint32_t>(std::max(0, iv));
        return true;
    }
    return false;
}

bool draw_bool(const reflect::FieldInfo& fi, void* ptr) {
    return ImGui::Checkbox(fi.name, static_cast<bool*>(ptr));
}

bool draw_vec2(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<glm::vec2*>(ptr);
    return ImGui::DragFloat2(fi.name, glm::value_ptr(v), 0.01f);
}

bool draw_vec3(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<glm::vec3*>(ptr);
    if (fi.flags & kFieldColor)
        return ImGui::ColorEdit3(fi.name, glm::value_ptr(v));
    return ImGui::DragFloat3(fi.name, glm::value_ptr(v), 0.01f);
}

bool draw_vec4(const reflect::FieldInfo& fi, void* ptr) {
    auto& v = *static_cast<glm::vec4*>(ptr);
    if (fi.flags & kFieldColor)
        return ImGui::ColorEdit4(fi.name, glm::value_ptr(v));
    return ImGui::DragFloat4(fi.name, glm::value_ptr(v), 0.01f);
}

bool draw_quat(const reflect::FieldInfo& fi, void* ptr) {
    auto& q = *static_cast<glm::quat*>(ptr);
    glm::vec3 euler = glm::degrees(glm::eulerAngles(q));
    if (ImGui::DragFloat3(fi.name, glm::value_ptr(euler), 0.5f)) {
        q = glm::quat(glm::radians(euler));
        return true;
    }
    return false;
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
WidgetDrawer::WidgetDrawer() {
    register_builtin_types();
    sort_and_freeze();
}

void WidgetDrawer::register_builtin_types() {
    table_.push_back({kTypeFloat,  draw_float});
    table_.push_back({kTypeDouble, draw_double});
    table_.push_back({kTypeInt32,  draw_int32});
    table_.push_back({kTypeUint32, draw_uint32});
    table_.push_back({kTypeBool,   draw_bool});
    table_.push_back({kTypeVec2,   draw_vec2});
    table_.push_back({kTypeVec3,   draw_vec3});
    table_.push_back({kTypeVec4,   draw_vec4});
    table_.push_back({kTypeQuat,   draw_quat});
}

void WidgetDrawer::sort_and_freeze() {
    std::sort(table_.begin(), table_.end());
    frozen_ = true;
}

void WidgetDrawer::register_type(uint32_t type_id, DrawFn fn) {
    // Silently no-op after freeze — registration is editor startup-only and
    // any late caller is a programming error, but the editor must not take
    // the sandbox `assert-on-error` path (see docs/12 §B2 — assertions are
    // sandbox-only). The misuse is visible via the developer console
    // (we log once) instead.
    if (frozen_) {
        static bool logged_once = false;
        if (!logged_once) {
            std::fprintf(stderr,
                "[widget_drawer] register_type(%u) called after freeze — ignored.\n",
                type_id);
            logged_once = true;
        }
        return;
    }
    table_.push_back({type_id, std::move(fn)});
}

bool WidgetDrawer::draw_field(const reflect::FieldInfo& fi, void* ptr) const {
    if (fi.flags & kFieldHidden) return false;

    ImGui::PushID(fi.name);

    auto it = std::lower_bound(table_.begin(), table_.end(), fi.type_id);
    bool changed = false;
    if (it != table_.end() && it->type_id == fi.type_id) {
        changed = it->fn(fi, ptr);
    } else {
        // Unknown type — show a hex dump of the raw bytes.
        ImGui::TextDisabled("%s: <type 0x%08x, %u bytes>",
            fi.name, fi.type_id, fi.size);
    }

    ImGui::PopID();
    return changed;
}

// ---------------------------------------------------------------------------
// Global singleton — valid for the editor's lifetime.
WidgetDrawer& get_widget_drawer() noexcept {
    static WidgetDrawer instance;
    return instance;
}

}  // namespace gw::editor
