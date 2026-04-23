//! `bld-tools::sacrilege_tools` — Part B §12.6 scaffold (ADR-0109).
//!
//! Seven descriptor-only copilot tools for Sacrilege. Each is registered via
//! `inventory::submit!` so the agent surface advertises them and the editor
//! Copilot panel can list them. Bodies land in Phase 23 / 28 (tracked by
//! the per-tool phase tag in `summary`).
//!
//! These stubs intentionally avoid the `#[bld_tool]` macro at this stage —
//! they carry the same `ToolDescriptor` wire shape so the taxonomy shows up
//! in `registry::all()` and the editor Copilot catalogue.

use crate::ToolDescriptor;
use bld_governance::tier::Tier;

macro_rules! sacrilege_tool {
    ($name:ident, $id:literal, $tier:expr, $summary:literal) => {
        #[doc(hidden)]
        pub static $name: ToolDescriptor = ToolDescriptor {
            id: $id,
            tier: $tier,
            summary: $summary,
        };
        inventory::submit!(&$name);
    };
}

sacrilege_tool!(
    CONCEPT_TO_MATERIAL,
    "sacrilege.concept_to_material",
    Tier::Mutate,
    "Concept image or prompt -> cooked .gwmat plus node graph (Phase 28)."
);
sacrilege_tool!(
    ENCOUNTER_SUGGEST,
    "sacrilege.encounter_suggest",
    Tier::Mutate,
    "Suggest enemy placements for a room / Circle profile / difficulty target (Phase 22)."
);
sacrilege_tool!(
    EXPLOIT_DETECT,
    "sacrilege.exploit_detect",
    Tier::Read,
    "Run the Director synthetic-player policy; flag loops, stuck cells, soft-locks (Phase 22)."
);
sacrilege_tool!(
    SCENE_HEATMAP,
    "sacrilege.scene_heatmap",
    Tier::Read,
    "Heat-map player-path frequency from recorded PIE runs (Phase 22)."
);
sacrilege_tool!(
    VOICE_LINE_GENERATE,
    "sacrilege.voice_line_generate",
    Tier::Mutate,
    "Seed-fed line generation for Malakor / Niccolo; HITL review required (Phase 22)."
);
sacrilege_tool!(
    DIALOGUE_REACHABILITY,
    "sacrilege.dialogue_reachability",
    Tier::Read,
    "Graph analysis over a .gwdlg; detects unreachable nodes and dead branches (Phase 22)."
);
sacrilege_tool!(
    DIRECTOR_TUNE,
    "sacrilege.director_tune",
    Tier::Mutate,
    "Search reward weights in the AI Director Sandbox with early stopping (Phase 26)."
);

#[cfg(test)]
mod tests {
    use super::*;
    use crate::registry;

    #[test]
    fn seven_sacrilege_tools_registered() {
        let ids = [
            "sacrilege.concept_to_material",
            "sacrilege.encounter_suggest",
            "sacrilege.exploit_detect",
            "sacrilege.scene_heatmap",
            "sacrilege.voice_line_generate",
            "sacrilege.dialogue_reachability",
            "sacrilege.director_tune",
        ];
        for id in ids {
            assert!(registry::find(id).is_some(), "missing tool: {id}");
        }
    }
}
