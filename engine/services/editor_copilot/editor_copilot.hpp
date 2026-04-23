#pragma once

#include "engine/services/editor_copilot/schema/copilot.hpp"

namespace gw::services::editor_copilot {

/// Editor-side BLD tool dispatch. Phase 27 wires this into editor/bld_bridge
/// (the C-ABI boundary). Runtime binaries never link this (BLD is editor-only).
[[nodiscard]] CopilotResult dispatch(const CopilotRequest& req) noexcept;

} // namespace gw::services::editor_copilot
