// engine/narrative/dialogue_graph.cpp — Phase 22 scaffold.

#include "engine/narrative/dialogue_graph.hpp"

namespace gw::narrative {

const DialogueLine* DialogueGraph::find(DialogueLineId id) const noexcept {
    for (const DialogueLine& ln : lines_) {
        if (ln.id == id) {
            return &ln;
        }
    }
    return nullptr;
}

} // namespace gw::narrative
