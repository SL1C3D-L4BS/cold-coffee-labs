#pragma once
// gameplay/martyrdom/martyrdom_register.hpp — Phase 22 W143
//
// Deterministic single-entry bootstrapper that registers the canonical
// Martyrdom component set with a World's ComponentRegistry in the order
// required by docs/07 §2.5. Always call once at gameplay_init() so the
// component ids are stable across the run.

#include "engine/ecs/world.hpp"
#include "gameplay/martyrdom/martyrdom_components.hpp"
#include "gameplay/martyrdom/stats.hpp"

#include <array>
#include <cstddef>

namespace gw::gameplay::martyrdom {

/// Canon registration order. Any change needs a save migration bump.
struct MartyrdomComponentIds {
    ecs::ComponentTypeId sin{};
    ecs::ComponentTypeId mantra{};
    ecs::ComponentTypeId rapture{};
    ecs::ComponentTypeId ruin{};
    ecs::ComponentTypeId resolved_stats{};
    ecs::ComponentTypeId active_blasphemies{};
    ecs::ComponentTypeId rapture_trigger_counter{};
};

[[nodiscard]] inline MartyrdomComponentIds register_martyrdom_components(ecs::World& world) noexcept {
    MartyrdomComponentIds ids{};
    auto& registry = world.component_registry();
    ids.sin                     = registry.id_of<SinComponent>();
    ids.mantra                  = registry.id_of<MantraComponent>();
    ids.rapture                 = registry.id_of<RaptureState>();
    ids.ruin                    = registry.id_of<RuinState>();
    ids.resolved_stats          = registry.id_of<ResolvedStats>();
    ids.active_blasphemies      = registry.id_of<ActiveBlasphemies>();
    ids.rapture_trigger_counter = registry.id_of<RaptureTriggerCounter>();
    return ids;
}

inline constexpr std::size_t kMartyrdomComponentCount = 7;

} // namespace gw::gameplay::martyrdom
