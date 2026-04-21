#pragma once
// editor/selection/selection_context.hpp
// Multi-entity selection set.
// Spec ref: Phase 7 §4 — "selection is UI state, not scene state" (no undo stack).

#include "engine/ecs/entity.hpp"

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace gw::editor {

// Editor-facing alias for the engine's 64-bit generational entity handle
// (ADR-0004 §2.2). Kept as `EntityHandle` so all the Phase-7 editor code
// that already spells the type this way continues to compile.
using EntityHandle = gw::ecs::Entity;
inline constexpr EntityHandle kNullEntity = gw::ecs::Entity::null();

class SelectionContext {
public:
    // Single-select: clear current selection and select h.
    void set(EntityHandle h);

    // Multi-select toggle: add if absent, remove if present.
    void toggle(EntityHandle h);

    void clear();

    [[nodiscard]] bool is_selected(EntityHandle h) const noexcept;

    [[nodiscard]] std::span<const EntityHandle> selected() const noexcept {
        return selected_;
    }

    // Primary = first selected entity (for inspector).
    [[nodiscard]] EntityHandle primary() const noexcept {
        return selected_.empty() ? kNullEntity : selected_.front();
    }

    [[nodiscard]] bool empty() const noexcept { return selected_.empty(); }
    [[nodiscard]] uint32_t count() const noexcept {
        return static_cast<uint32_t>(selected_.size());
    }

    // Pivot for gizmo: primary selection's entity (real world-position lookup
    // is the gizmo's job; this just returns which entity to query).
    [[nodiscard]] EntityHandle pivot_entity() const noexcept {
        return selected_.empty() ? kNullEntity : selected_.front();
    }

    // Signal — fires whenever the selection changes.
    // Panels subscribe by pushing a callback; cleared on ~EditorApplication.
    void subscribe(std::function<void(const SelectionContext&)> cb) {
        listeners_.push_back(std::move(cb));
    }

private:
    void notify() const;

    std::vector<EntityHandle>                              selected_;
    std::vector<std::function<void(const SelectionContext&)>> listeners_;
};

}  // namespace gw::editor
