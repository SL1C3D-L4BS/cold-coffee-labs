#pragma once
// editor/scene/transform_system.hpp
// Phase 8 week 044 — local→world matrix propagation for the scene graph.
//
// The transform system consumes `HierarchyComponent + TransformComponent` and
// produces `WorldMatrixComponent` on every entity that has a Transform.
// World matrices are `glm::dmat4` (float64) so that under CLAUDE.md
// non-negotiable #17 ("world-space positions use float64"), a tree of
// transforms can be composed with sub-millimetre precision across planet
// scale without accumulating float32 round-off in the parent chain.
//
// The implementation is O(n) per `update_transforms` call: collect all
// `TransformComponent` rows, Kahn topologically order by `HierarchyComponent`
// parent links (with cycle fallback to local-only), then compose world
// `glm::dmat4` in sorted order. A future optimization can add incremental
// (dirty-subtree) updates; the public API is stable.
//
// Engine layering note: this file intentionally lives under `editor/scene/`
// for Phase 8 because `TransformComponent` itself still lives there (a
// pending Phase 9 chore moves both into `engine/scene/` once GW_REFLECT has
// migrated to `engine/core/`). The header has zero ImGui / GLFW deps so the
// move is a pure rename when it lands.

#include "engine/ecs/world.hpp"

#include <cstddef>

namespace gw::editor::scene {

// Build every missing WorldMatrixComponent and compose absolute-world
// matrices for entities with a TransformComponent. Returns the number of
// entities whose WorldMatrixComponent was written this call.
//
// Must be called after hierarchy edits and before any consumer that reads
// WorldMatrixComponent (rendering, gizmo, physics, picking). Safe to call
// every frame — the pass is cache-friendly and allocation-free on the hot
// path (internal buffers are thread_local).
std::size_t update_transforms(gw::ecs::World& w);

} // namespace gw::editor::scene
