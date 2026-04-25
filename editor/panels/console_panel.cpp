// editor/panels/console_panel.cpp
#include "editor/panels/console_panel.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

#include <imgui.h>

namespace gw::editor {
namespace {

[[nodiscard]] const char* level_abbrev(const gw::core::LogLevel l) noexcept {
    using gw::core::LogLevel;
    switch (l) {
        case LogLevel::Trace: return "T";
        case LogLevel::Info:  return "I";
        case LogLevel::Warn:  return "W";
        case LogLevel::Error: return "E";
    }
    return "?";
}

ImVec4 level_color(const gw::core::LogLevel l) noexcept {
    using gw::core::LogLevel;
    switch (l) {
        case LogLevel::Trace: return ImVec4(0.55f, 0.55f, 0.60f, 1.f);
        case LogLevel::Info:  return ImVec4(0.80f, 0.90f, 0.80f, 1.f);
        case LogLevel::Warn:  return ImVec4(0.95f, 0.80f, 0.45f, 1.f);
        case LogLevel::Error: return ImVec4(0.95f, 0.45f, 0.45f, 1.f);
    }
    return ImVec4(1.f, 1.f, 1.f, 1.f);
}

void tolower_inplace(std::string& s) noexcept {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
}

[[nodiscard]] bool match_filter(
    const std::string_view filter_lower, const std::string_view category, const std::string_view message) noexcept {
    if (filter_lower.empty()) {
        return true;
    }
    std::string c{category};
    std::string m{message};
    tolower_inplace(c);
    tolower_inplace(m);
    return c.find(filter_lower) != std::string::npos || m.find(filter_lower) != std::string::npos;
}

} // namespace

void ConsolePanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted("Reads gw::core::log ring. Set GW_LOG_FILE for a file append sink. "
                           "Tracy: run the Tracy viewer against the host/port below; enable the "
                           "Tracy client in engine builds with GW_TRACY=1 (see CMake).");

    if (const char* p = std::getenv("GW_LOG_FILE")) {
        ImGui::TextDisabled("GW_LOG_FILE=%s", p);
    }

    ImGui::InputText("Filter (substring)", filter_.data(), filter_.size());
    const char* const fb   = filter_.data();
    const std::size_t flen = strnlen(fb, filter_.size());
    std::string       filter_l;
    filter_l.assign(fb, flen);
    tolower_inplace(filter_l);

    ImGui::Checkbox("Pause (freeze list)", &paused_);
    if (paused_ && !last_pause_) {
        snapshot_.clear();
        gw::core::log_for_each_line(
            this, [](void* p, const gw::core::LogLineEntry& e) noexcept {
                static_cast<ConsolePanel*>(p)->snapshot_.push_back(e);
            });
    }
    last_pause_ = paused_;
    ImGui::SameLine();
    if (ImGui::Button("Clear##log")) {
        gw::core::log_clear_buffer();
        snapshot_.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy##log")) {
        std::string text;
        if (paused_) {
            for (const auto& e : snapshot_) {
                if (!match_filter(filter_l,
                        std::string_view{e.category.c_str(), e.category.size()},
                        std::string_view{e.message.c_str(), e.message.size()})) {
                    continue;
                }
                text += '[';
                text += level_abbrev(e.level);
                text += "][";
                text += e.category;
                text += "] ";
                text += e.message;
                text += '\n';
            }
        } else {
            struct Acc {
                std::string*    text;
                std::string_view fl;
            } acc{&text, filter_l};
            gw::core::log_for_each_line(
                &acc, +[](void* p, const gw::core::LogLineEntry& e) noexcept {
                    const auto& a  = *static_cast<Acc*>(p);
                    const char* c  = e.category.c_str();
                    const char* m  = e.message.c_str();
                    const size_t   csz = e.category.size();
                    const size_t   msz = e.message.size();
                    if (!match_filter(a.fl, {c, csz}, {m, msz})) {
                        return;
                    }
                    *a.text += '[';
                    *a.text += level_abbrev(e.level);
                    *a.text += "][";
                    *a.text += e.category;
                    *a.text += "] ";
                    *a.text += e.message;
                    a.text->push_back('\n');
                });
        }
        if (!text.empty()) {
            ImGui::SetClipboardText(text.c_str());
        }
    }

    if (ImGui::CollapsingHeader("Tracy", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Viewer host", tracy_host_.data(), tracy_host_.size());
        ImGui::InputInt("Port", &tracy_port_);
        if (tracy_port_ < 1) {
            tracy_port_ = 1;
        }
        if (tracy_port_ > 65535) {
            tracy_port_ = 65535;
        }
        if (ImGui::Button("Log connect hint")) {
            std::string msg = "Tracy: open viewer, connect to ";
            msg += tracy_host_.data();
            msg += " port ";
            msg += std::to_string(tracy_port_);
            msg += " (app must be built with Tracy + GW_TRACY).";
            gw::core::log_message(
                gw::core::LogLevel::Info, "tracy", std::string_view{msg});
        }
    }

    const float footer = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("log_scroll", ImVec2(0, -footer), true, ImGuiWindowFlags_HorizontalScrollbar);
    if (paused_) {
        for (const auto& e : snapshot_) {
            if (!match_filter(filter_l,
                    std::string_view{e.category.c_str(), e.category.size()},
                    std::string_view{e.message.c_str(), e.message.size()})) {
                continue;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, level_color(e.level));
            const std::string line = std::string("["  ) + level_abbrev(e.level) + "][" + e.category + "] " + e.message;
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
    } else {
        struct Row {
            std::string_view f;
        } row{std::string_view{filter_l}};
        gw::core::log_for_each_line(
            &row, +[](void* p, const gw::core::LogLineEntry& e) noexcept {
                const auto& rf = *static_cast<Row*>(p);
                const char*  c  = e.category.c_str();
                const char*  m  = e.message.c_str();
                const size_t  csz = e.category.size();
                const size_t  msz = e.message.size();
                if (!match_filter(rf.f, {c, csz}, {m, msz})) {
                    return;
                }
                ImGui::PushStyleColor(ImGuiCol_Text, level_color(e.level));
                const std::string line = std::string("["  ) + level_abbrev(e.level) + "][" + e.category + "] " + e.message;
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            });
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace gw::editor
