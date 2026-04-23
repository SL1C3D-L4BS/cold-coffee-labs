// engine/services/editor_copilot/editor_copilot.cpp — Phase 27 scaffold.

#include "engine/services/editor_copilot/editor_copilot.hpp"

namespace gw::services::editor_copilot {

CopilotResult dispatch(const CopilotRequest& /*req*/) noexcept {
    CopilotResult out{};
    out.ok = false;
    // Phase 27: forward to bld/bld-tools via the editor C-ABI. Shim returns
    // a not-wired sentinel so editor panels can render a graceful "BLD not
    // available" state.
    return out;
}

} // namespace gw::services::editor_copilot
