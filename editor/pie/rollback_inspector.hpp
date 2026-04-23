#pragma once
// editor/pie/rollback_inspector.hpp — Part B §12.5 scaffold.
//
// Step the deterministic simulation forward/back; replay seed field. Wires
// to engine/world/universe/determinism_validator so the scrubber honours
// the same bit-hash contract used in CI.

#include <cstdint>

namespace gw::editor::pie {

struct RollbackInspectorState {
    std::uint64_t seed           = 0;
    std::uint64_t tick           = 0;
    std::uint64_t bit_hash       = 0;   // current visited-state hash
    bool          paused         = true;
    int           step_size      = 1;
};

void draw_rollback_inspector(RollbackInspectorState& state) noexcept;

} // namespace gw::editor::pie
