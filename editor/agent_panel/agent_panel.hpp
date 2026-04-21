#pragma once
// editor/agent_panel/agent_panel.hpp
// AgentPanel — the BLD chat surface (ADR-0011, ADR-0014, ADR-0015).
// Phase 9A scaffolds the panel; Wave 9D wires the live agent loop
// through the Rust bld-bridge.
//
// This is a pure-ImGui panel:
// - Left: scrollable transcript (role-coloured bubbles).
// - Bottom: multi-line input + Send button.
// - Top-right: provider picker + status dot + stop button.
// - Elicitation modal overlays when BLD requests approval.
//
// The panel intentionally has zero dependencies on the Rust side at this
// phase — every public method is non-virtual and returns immediately.
// The transport is owned by `AgentSession` (see agent_session.hpp) which
// in Wave 9D will drive the bridge. Keeping the panel dumb guarantees
// the editor compiles and renders even when BLD isn't attached.

#include "editor/panels/panel.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace gw::editor::agent {

// ---------------------------------------------------------------------------
// Message / turn model (mirrors bld_provider::ChatMessage for wire parity).
// ---------------------------------------------------------------------------
enum class MsgRole : uint8_t { User, Assistant, Tool, System };

struct Message {
    MsgRole     role   = MsgRole::User;
    std::string body;
    // Non-empty when role == Tool. Stable identifier the model emitted.
    std::string tool_call_id;
    std::string tool_name;
};

enum class SessionStatus : uint8_t {
    Idle,
    Thinking,     // awaiting provider response
    Acting,       // dispatching a tool call
    Awaiting,     // waiting on user elicitation
    Disconnected, // BLD library not loaded
};

// One pending elicitation request. Wired by Wave 9E when the bridge
// pushes elicitation requests into the panel.
struct ElicitationRequest {
    std::string tool_id;
    std::string prompt_markdown;
    bool        expects_form = true;  // vs URL-mode for secrets
};

// ---------------------------------------------------------------------------
// AgentPanel
// ---------------------------------------------------------------------------
class AgentPanel final : public IPanel {
public:
    AgentPanel();

    void on_imgui_render(EditorContext& ctx) override;

    [[nodiscard]] const char* name() const override { return "BLD Copilot"; }

    // ------------------------------------------------------------------
    // Transcript management. Wave 9D calls these from the bridge.
    // ------------------------------------------------------------------
    void push_user(std::string_view text);
    void push_assistant_delta(std::string_view text);
    void push_tool_result(std::string_view tool, std::string_view result);
    void set_status(SessionStatus s) noexcept { status_ = s; }

    // Present an elicitation request. Blocks interaction until user acts.
    void push_elicitation(ElicitationRequest req);

    // Drain the current user input. Wave 9D polls this to feed the bridge.
    [[nodiscard]] bool take_pending_user_input(std::string& out);

    // Cancel button state — returns true if the user clicked "Stop" since
    // the last poll. Drains after reading.
    [[nodiscard]] bool take_stop_flag() noexcept;

private:
    // Transcript bubble rendering.
    void render_transcript();
    void render_input_area();
    void render_elicitation_modal();
    void render_header_bar();

    // UI state.
    std::deque<Message> transcript_;
    std::string         input_buffer_;   // mutable char[] for ImGui::InputTextMultiline
    SessionStatus       status_          = SessionStatus::Disconnected;
    bool                pending_input_   = false;
    bool                stop_flag_       = false;
    bool                autoscroll_      = true;

    // Elicitation overlay.
    std::vector<ElicitationRequest> elicitations_;
    bool                elicitation_form_approved_ = false;
    bool                elicitation_form_denied_   = false;

    // Provider picker ("Claude Opus 4.7 (cloud)" / "mistral.rs (local)" / "llama-cpp (local)").
    int                 provider_index_  = 0;
    static constexpr const char* kProviders[] = {
        "Claude Opus 4.7 (cloud)",
        "mistral.rs (local)",
        "llama-cpp (local)",
        "Offline",
    };

    // Visual metrics.
    static constexpr float kInputRowHeight  = 120.f;
    static constexpr float kBubbleRounding  = 6.f;
    static constexpr float kBubblePadding   = 10.f;
};

}  // namespace gw::editor::agent
