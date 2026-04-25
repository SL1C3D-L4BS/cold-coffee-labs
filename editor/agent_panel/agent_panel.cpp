// editor/agent_panel/agent_panel.cpp
// AgentPanel — implementation.
//
// Phase 9A delivers the chat UI without the live bridge: transcript
// display, input area, provider picker, status indicator, and the
// elicitation modal. All state lives inside the panel and is polled by
// the editor in Wave 9D once the bld-bridge session is ready.

#include "agent_panel.hpp"
#include "editor/theme/palette_imgui.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace gw::editor::agent {

namespace {

ImVec4 role_accent(MsgRole r) {
    using gw::editor::theme::active_accent_secondary_imgui;
    using gw::editor::theme::active_info_imgui;
    using gw::editor::theme::active_link_imgui;
    using gw::editor::theme::active_muted_imgui;
    using gw::editor::theme::active_warning_imgui;
    switch (r) {
        case MsgRole::User:      return active_info_imgui();
        case MsgRole::Assistant: return active_link_imgui();
        case MsgRole::Tool:      return active_warning_imgui();
        case MsgRole::System:    return active_muted_imgui();
    }
    return active_accent_secondary_imgui();
}

const char* role_label(MsgRole r) {
    switch (r) {
        case MsgRole::User:      return "you";
        case MsgRole::Assistant: return "bld";
        case MsgRole::Tool:      return "tool";
        case MsgRole::System:    return "sys";
    }
    return "?";
}

const char* status_label(SessionStatus s) {
    switch (s) {
        case SessionStatus::Idle:         return "idle";
        case SessionStatus::Thinking:     return "thinking";
        case SessionStatus::Acting:       return "acting";
        case SessionStatus::Awaiting:     return "awaiting you";
        case SessionStatus::Disconnected: return "disconnected";
    }
    return "?";
}

[[nodiscard]] const char* status_label_for_provider(SessionStatus s, int provider_index) {
    if (provider_index == AgentPanel::k_provider_index_mcp_tools) {
        switch (s) {
        case SessionStatus::Disconnected:
            return "mcp: set GW_BLD_SERVER_EXE";
        case SessionStatus::Idle:
            return "mcp ready";
        case SessionStatus::Thinking:
            return "mcp busy";
        case SessionStatus::Acting:
            return "mcp acting";
        case SessionStatus::Awaiting:
            return "mcp awaiting you";
        }
    }
    return status_label(s);
}

ImVec4 status_accent(SessionStatus s) {
    using gw::editor::theme::active_accent_secondary_imgui;
    using gw::editor::theme::active_accent_strong_imgui;
    using gw::editor::theme::active_muted_imgui;
    using gw::editor::theme::active_positive_imgui;
    using gw::editor::theme::active_warning_imgui;
    switch (s) {
        case SessionStatus::Idle:         return active_positive_imgui();
        case SessionStatus::Thinking:     return active_warning_imgui();
        case SessionStatus::Acting:       return active_accent_strong_imgui();
        case SessionStatus::Awaiting:     return active_accent_secondary_imgui();
        case SessionStatus::Disconnected: return active_muted_imgui();
    }
    return active_accent_secondary_imgui();
}

}  // namespace

AgentPanel::AgentPanel() {
    input_buffer_.reserve(2048);
    // Seed a greeting so first launch isn't blank.
    transcript_.push_back(Message{
        .role         = MsgRole::System,
        .body         = "Brewed Logic Directive. Pick \"MCP — gw_bld_server\" and set "
                        "environment variable GW_BLD_SERVER_EXE to run `docs.search` on the "
                        "open project; other providers remain offline until wired.",
        .tool_call_id = {},
        .tool_name    = {},
    });
}

void AgentPanel::push_user(std::string_view text) {
    transcript_.push_back(Message{
        .role = MsgRole::User, .body = std::string{text},
        .tool_call_id = {}, .tool_name = {}
    });
}

void AgentPanel::push_assistant_delta(std::string_view text) {
    if (transcript_.empty() || transcript_.back().role != MsgRole::Assistant) {
        transcript_.push_back(Message{
            .role = MsgRole::Assistant, .body = std::string{text},
            .tool_call_id = {}, .tool_name = {}
        });
    } else {
        transcript_.back().body.append(text);
    }
}

void AgentPanel::push_tool_result(std::string_view tool, std::string_view result) {
    transcript_.push_back(Message{
        .role         = MsgRole::Tool,
        .body         = std::string{result},
        .tool_call_id = {},
        .tool_name    = std::string{tool},
    });
}

void AgentPanel::push_elicitation(ElicitationRequest req) {
    elicitations_.push_back(std::move(req));
    status_ = SessionStatus::Awaiting;
}

bool AgentPanel::take_pending_user_input(std::string& out) {
    if (!pending_input_) {
        return false;
    }
    out = std::move(pending_mcp_line_);
    pending_input_ = false;
    return !out.empty();
}

bool AgentPanel::take_stop_flag() noexcept {
    const bool f = stop_flag_;
    stop_flag_   = false;
    return f;
}

void AgentPanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) return;

    ImGui::SetNextWindowSize(ImVec2{480.f, 640.f}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(name(), &visible_)) {
        ImGui::End();
        return;
    }

    render_header_bar();
    ImGui::Separator();

    const float input_reserved = kInputRowHeight + ImGui::GetStyle().ItemSpacing.y + 8.f;
    if (ImGui::BeginChild("##transcript",
                          ImVec2{0.f, -input_reserved},
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        render_transcript();
    }
    ImGui::EndChild();

    render_input_area();
    render_elicitation_modal();

    ImGui::End();
}

void AgentPanel::render_header_bar() {
    if (provider_index_ < 0 || provider_index_ >= IM_ARRAYSIZE(kProviders)) {
        provider_index_ = 0;
    }
    // Provider picker.
    ImGui::SetNextItemWidth(220.f);
    if (ImGui::BeginCombo("##provider", kProviders[provider_index_])) {
        for (int i = 0; i < IM_ARRAYSIZE(kProviders); ++i) {
            const bool selected = (i == provider_index_);
            if (ImGui::Selectable(kProviders[i], selected)) {
                provider_index_ = i;
                if (i == AgentPanel::k_provider_index_mcp_tools) {
                    const char* e = std::getenv("GW_BLD_SERVER_EXE");
                    status_ = (e != nullptr && e[0] != '\0') ? SessionStatus::Idle
                                                           : SessionStatus::Disconnected;
                } else {
                    status_ = SessionStatus::Idle;
                }
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Status dot + label.
    ImGui::SameLine();
    const ImVec4 col = status_accent(status_);
    ImGui::TextColored(col, "o");
    ImGui::SameLine(0.f, 4.f);
    // Use the same accent as the dot — TextDisabled reads like a broken link
    // even when status is Idle / mcp ready.
    ImGui::TextColored(col, "%s", status_label_for_provider(status_, provider_index_));

    // Stop / Clear buttons — right-aligned.
    ImGui::SameLine();
    const float right_edge = ImGui::GetContentRegionAvail().x;
    if (right_edge > 140.f)
        ImGui::Dummy(ImVec2{right_edge - 140.f, 1.f});
    ImGui::SameLine();
    ImGui::BeginDisabled(status_ != SessionStatus::Thinking &&
                         status_ != SessionStatus::Acting);
    if (ImGui::SmallButton("Stop")) stop_flag_ = true;
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) {
        transcript_.clear();
        transcript_.push_back(Message{
            .role = MsgRole::System, .body = "new session.",
            .tool_call_id = {}, .tool_name = {}
        });
        if (provider_index_ == k_provider_index_mcp_tools) {
            const char* e = std::getenv("GW_BLD_SERVER_EXE");
            status_ = (e != nullptr && e[0] != '\0') ? SessionStatus::Idle
                                                   : SessionStatus::Disconnected;
        } else {
            status_ = SessionStatus::Idle;
        }
    }
}

void AgentPanel::render_transcript() {
    for (const Message& m : transcript_) {
        const ImVec4 accent = role_accent(m.role);

        ImGui::PushStyleColor(
            ImGuiCol_ChildBg,
            gw::editor::theme::imgui_vec4(
                gw::editor::theme::ThemeRegistry::instance().active().palette.surface_1));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kBubbleRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2{kBubblePadding, kBubblePadding});

        const float bubble_w = std::min(ImGui::GetContentRegionAvail().x, 640.f);
        // The transcript bubble height is unbounded; we pass 0 so ImGui
        // expands to fit the wrapped text. We use PushTextWrapPos to keep
        // long model replies from sprawling beyond the panel's content
        // region.
        ImGui::BeginChild(ImGui::GetID(&m),
                          ImVec2{bubble_w, 0.f},
                          ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
        ImGui::TextColored(accent, "%s", role_label(m.role));
        if (m.role == MsgRole::Tool && !m.tool_name.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("[%s]", m.tool_name.c_str());
        }
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + bubble_w - kBubblePadding * 2);
        ImGui::TextUnformatted(m.body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Spacing();
    }

    if (autoscroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.f)
        ImGui::SetScrollHereY(1.f);
}

void AgentPanel::render_input_area() {
    // Ensure the backing string always has capacity for ImGui editing.
    if (input_buffer_.capacity() < 2048) input_buffer_.reserve(2048);

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput |
                                ImGuiInputTextFlags_CtrlEnterForNewLine;

    // ImGui requires a char*, so we point into the std::string and rely
    // on capacity. Resize to fit whatever ImGui wrote.
    auto resize_cb = [](ImGuiInputTextCallbackData* data) -> int {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            auto* s = static_cast<std::string*>(data->UserData);
            s->resize(static_cast<size_t>(data->BufSize));
            data->Buf = s->data();
        }
        return 0;
    };
    flags |= ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue;

    // Make sure the underlying buffer holds at least one null terminator
    // for ImGui to find.
    if (input_buffer_.empty()) input_buffer_.push_back('\0');

    const float avail_w  = ImGui::GetContentRegionAvail().x;
    const float input_w  = avail_w - 80.f;

    const bool submitted = ImGui::InputTextMultiline(
        "##input",
        input_buffer_.data(),
        input_buffer_.capacity(),
        ImVec2{input_w, kInputRowHeight},
        flags, resize_cb, &input_buffer_);

    // Normalise to remove stray trailing nulls.
    if (!input_buffer_.empty() && input_buffer_.back() == '\0')
        input_buffer_.pop_back();

    ImGui::SameLine();
    const bool can_send = status_ != SessionStatus::Thinking &&
                          status_ != SessionStatus::Acting;
    ImGui::BeginDisabled(!can_send);
    const bool clicked = ImGui::Button("Send",
                                        ImVec2{74.f, kInputRowHeight});
    ImGui::EndDisabled();

    if ((submitted || clicked) && !input_buffer_.empty()) {
        std::string line = input_buffer_;
        while (!line.empty() && line.back() == '\0') {
            line.pop_back();
        }
        push_user(line);
        if (provider_index_ == AgentPanel::k_provider_index_mcp_tools) {
            pending_input_   = true;
            pending_mcp_line_ = std::move(line);
        }
        input_buffer_.clear();
        input_buffer_.push_back('\0');
    }
}

void AgentPanel::render_elicitation_modal() {
    if (elicitations_.empty()) return;

    ImGui::OpenPopup("BLD requests approval##elicit");
    if (ImGui::BeginPopupModal("BLD requests approval##elicit",
                                nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const ElicitationRequest& req = elicitations_.front();
        ImGui::TextColored(gw::editor::theme::active_warning_imgui(), "Tool: %s",
                           req.tool_id.c_str());
        ImGui::Separator();
        ImGui::PushTextWrapPos(480.f);
        ImGui::TextUnformatted(req.prompt_markdown.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Separator();

        if (ImGui::Button("Approve once", ImVec2{140.f, 0.f})) {
            elicitation_form_approved_ = true;
            elicitations_.erase(elicitations_.begin());
            if (elicitations_.empty()) status_ = SessionStatus::Idle;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Approve for session", ImVec2{160.f, 0.f})) {
            elicitation_form_approved_ = true;
            elicitations_.erase(elicitations_.begin());
            if (elicitations_.empty()) status_ = SessionStatus::Idle;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Deny", ImVec2{100.f, 0.f})) {
            elicitation_form_denied_ = true;
            elicitations_.erase(elicitations_.begin());
            if (elicitations_.empty()) status_ = SessionStatus::Idle;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}  // namespace gw::editor::agent
