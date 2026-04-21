// engine/ui/settings_binder.cpp — ADR-0027 Wave 11E.

#include "engine/ui/settings_binder.hpp"
#include "engine/core/config/cvar_registry.hpp"

#include <cstdio>

namespace gw::ui {

void SettingsBinder::bind(const SettingsBinding& b) {
    for (auto& existing : bindings_) {
        if (existing.element_id == b.element_id) {
            existing = b;
            return;
        }
    }
    bindings_.push_back(b);
}

void SettingsBinder::clear() { bindings_.clear(); }

std::string_view SettingsBinder::cvar_for(std::string_view element_id) const noexcept {
    for (const auto& b : bindings_) {
        if (b.element_id == element_id) return b.cvar_name;
    }
    return {};
}

bool SettingsBinder::push_from_ui(std::string_view element_id, std::string_view new_value) {
    auto name = cvar_for(element_id);
    if (name.empty()) return false;
    using R = config::CVarRegistry::SetResult;
    return cvars_.set_from_string(name, new_value, config::kOriginRmlBinding) == R::Ok;
}

std::string SettingsBinder::pull_for_ui(std::string_view element_id) const {
    auto name = cvar_for(element_id);
    if (name.empty()) return {};
    auto* e = cvars_.entry_by_name(name);
    if (!e) return {};
    switch (e->type) {
        case config::CVarType::Bool:   return e->v_bool ? "true" : "false";
        case config::CVarType::Int32:
        case config::CVarType::Enum:   return std::to_string(e->v_i32);
        case config::CVarType::Int64:  return std::to_string(e->v_i64);
        case config::CVarType::Float: {
            char buf[32]; std::snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(e->v_f32));
            return buf;
        }
        case config::CVarType::Double: {
            char buf[40]; std::snprintf(buf, sizeof(buf), "%.9g", e->v_f64);
            return buf;
        }
        case config::CVarType::String: return e->v_str;
    }
    return {};
}

} // namespace gw::ui
