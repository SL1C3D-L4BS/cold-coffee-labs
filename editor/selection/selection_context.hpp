#pragma once
// editor/selection/selection_context.hpp
// Multi-entity selection set.
// Spec ref: Phase 7 §4 — "selection is UI state, not scene state" (no undo stack).

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

// Forward-declare instead of pulling in the full ECS world.
namespace gw::ecs { struct EntityHandle; }

namespace gw::editor {

// Opaque 64-bit entity handle matching engine/ecs (forwards-compatible).
// Phase 7 uses uint64_t; Phase 8 will replace with ecs::EntityHandle directly.
using EntityHandle = uint64_t;
inline constexpr EntityHandle kNullEntity = 0u;

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

    // Pivot for gizmo: simple arithmetic mean of entity indices.
    // A real implementation queries world positions (Phase 8+).
    // Phase 7: pivot = mean of entity handle values cast to a placeholder.
    [[nodiscard]] uint64_t pivot_entity() const noexcept {
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
