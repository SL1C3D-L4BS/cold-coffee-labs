#pragma once
// engine/ui/settings_binder.hpp — RmlUi ↔ CVarRegistry two-way binding (ADR-0027).
//
// Runs outside the GW_UI_RMLUI gate so tests can exercise the bind logic
// against a fake document model. The RmlUi-enabled build uses these hooks
// to plug into Rml::DataModel / Rml::DataEventListener.

#include "engine/core/config/cvar.hpp"
#include "engine/ui/ui_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gw::config { class CVarRegistry; }
namespace gw::events { struct ConfigCVarChanged; }

namespace gw::ui {

// One binding row: associates a RmlUi element (`element_id`) with a CVar.
struct SettingsBinding {
    std::string     element_id{};   // CSS id of the <input>/<select>/<toggle>
    std::string     cvar_name{};
    std::uint32_t   flags{0};       // reserved — e.g. kBindOneWay
};

class SettingsBinder {
public:
    explicit SettingsBinder(config::CVarRegistry& registry) : cvars_(registry) {}
    ~SettingsBinder() = default;
    SettingsBinder(const SettingsBinder&) = delete;
    SettingsBinder& operator=(const SettingsBinder&) = delete;

    void bind(const SettingsBinding& b);
    void clear();
    [[nodiscard]] std::size_t binding_count() const noexcept { return bindings_.size(); }

    // Called by the RmlUi event listener when the user edits a control;
    // pushes a write into the CVar registry with kOriginRmlBinding.
    // Returns true on success.
    bool push_from_ui(std::string_view element_id, std::string_view new_value);

    // Called by the CVar change handler; returns the new string value an
    // element should display. Empty string on miss.
    [[nodiscard]] std::string pull_for_ui(std::string_view element_id) const;

    // Look up the cvar name bound to an element. Empty view on miss.
    [[nodiscard]] std::string_view cvar_for(std::string_view element_id) const noexcept;

    [[nodiscard]] const std::vector<SettingsBinding>& bindings() const noexcept {
        return bindings_;
    }

private:
    config::CVarRegistry&           cvars_;
    std::vector<SettingsBinding>    bindings_{};
};

} // namespace gw::ui
