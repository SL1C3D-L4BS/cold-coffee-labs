#pragma once
// engine/persist/persist_world.hpp — Phase 15 PIMPL facade (ADR-0056).

#include "engine/persist/cloud_save.hpp"
#include "engine/persist/events_persist.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/persist/persist_config.hpp"
#include "engine/persist/persist_types.hpp"

#include <memory>

namespace gw::config {
class CVarRegistry;
}
namespace gw::ecs {
class World;
}
namespace gw::events {
class CrossSubsystemBus;
}
namespace gw::jobs {
class Scheduler;
}

namespace gw::persist {

class PersistWorld {
public:
    PersistWorld();
    ~PersistWorld();

    PersistWorld(const PersistWorld&)            = delete;
    PersistWorld& operator=(const PersistWorld&) = delete;
    PersistWorld(PersistWorld&&)                 = delete;
    PersistWorld& operator=(PersistWorld&&)      = delete;

    [[nodiscard]] bool initialize(PersistConfig cfg,
                                  config::CVarRegistry* cvars,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler*           scheduler = nullptr);
    void shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    [[nodiscard]] SaveError save_slot(SlotId id, ecs::World& w, const SaveMeta& meta);
    [[nodiscard]] LoadError load_slot(SlotId id, ecs::World& w);
    /// Recomputes determinism hash from centre chunk vs header.
    [[nodiscard]] LoadError validate_slot(SlotId id, ecs::World& w);
    /// Bumps on-disk `schema_version` in header to `kCurrentSaveSchemaVersion` and rewrites footer.
    [[nodiscard]] SaveError migrate_slot(SlotId id);

    void step(double dt_seconds);

    [[nodiscard]] ILocalStore*       local_store() noexcept;
    [[nodiscard]] ICloudSave*        cloud() noexcept;
    [[nodiscard]] const PersistConfig& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::persist
