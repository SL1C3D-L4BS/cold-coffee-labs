// apps/sandbox_persistence/main.cpp — Phase 15 exit gate (ADR-0064 §12).

#include "runtime/engine.hpp"

#include "engine/core/crash_reporter.hpp"
#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"
#include "engine/persist/cloud_save.hpp"
#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/persist_types.hpp"
#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/dsar_exporter.hpp"

#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace {

struct Pos {
    float x = 0, y = 0, z = 0;
};
struct Vel {
    float dx = 0, dy = 0, dz = 0;
};
struct Health {
    float hp = 100.f;
};

} // namespace

int main() {
    gw::core::crash::install_handlers();
    gw::runtime::EngineConfig ecfg{};
    ecfg.headless       = true;
    ecfg.deterministic  = true;
    ecfg.self_test_frames = 1;
    gw::runtime::Engine engine(ecfg);

    auto& reg = engine.cvars();
    if (const auto id = reg.find("tele.enabled"); id.valid()) {
        (void)reg.set_bool(id, true);
    }
    if (const auto id = reg.find("tele.events.enabled"); id.valid()) {
        (void)reg.set_bool(id, true);
    }

    gw::ecs::World world;
    (void)world.component_registry().id_of<Pos>();
    (void)world.component_registry().id_of<Vel>();
    (void)world.component_registry().id_of<Health>();

    for (int i = 0; i < 2048; ++i) {
        const auto e = world.create_entity();
        world.add_component(e, Pos{static_cast<float>(i), 0.f, 0.f});
        world.add_component(e, Vel{1.f, 0.f, 0.f});
        world.add_component(e, Health{100.f});
    }

    auto& pw = engine.persist();
    gw::persist::SaveMeta meta{};
    meta.display_name     = "sandbox";
    meta.world_seed       = 0xC0FFEEu;
    meta.session_seed     = 0xDECAFBADu;
    meta.created_unix_ms  = 1;
    meta.schema_version   = 1; // migrate will bump file to kCurrent
    meta.consent_tier_snapshot = "None";

    int saves      = 0;
    int migrations = 0;
    int cloud_rt   = 0;

    if (pw.save_slot(1, world, meta) == gw::persist::SaveError::Ok) ++saves;
    if (pw.save_slot(2, world, meta) == gw::persist::SaveError::Ok) ++saves;

    if (pw.migrate_slot(1) == gw::persist::SaveError::Ok) ++migrations;

    gw::ecs::World validate_world;
    (void)validate_world.component_registry().id_of<Pos>();
    (void)validate_world.component_registry().id_of<Vel>();
    (void)validate_world.component_registry().id_of<Health>();
    const auto v = pw.validate_slot(1, validate_world);
    (void)v;

    if (pw.cloud()) {
        std::vector<std::byte> bytes;
        gw::persist::SlotStamp st{};
        st.unix_ms       = 10;
        st.vector_clock  = 1;
        st.machine_salt  = 0x1234u;
        const auto p1    = pw.cloud()->put(1, std::span<const std::byte>(), st);
        if (p1 == gw::persist::CloudError::Ok) ++cloud_rt;
        if (pw.cloud()->get(1, bytes, st) == gw::persist::CloudError::Ok) ++cloud_rt;
    }

    auto* store = pw.local_store();
    if (store) {
        gw::telemetry::consent_store(*store, gw::telemetry::ConsentTier::CrashOnly, "2026-04");
    }
    engine.telemetry().set_consent_tier(gw::telemetry::ConsentTier::CoreTelemetry);

    int events = 0;
    for (int i = 0; i < 500; ++i) {
        if (engine.telemetry().record_event("session_heartbeat", "{\"i\":0}") == gw::telemetry::TelemetryError::Ok) {
            ++events;
        }
    }
    (void)engine.telemetry().record_event("tele_test_crash", "{}");
    (void)engine.telemetry().flush();

    std::uint64_t dsar_bytes = 0;
    if (store) {
        const std::string dsar_dir = "dsar_out";
        if (gw::telemetry::dsar_export_to_dir(dsar_dir, *store, "sandboxhash")) {
            dsar_bytes = 1;
        }
    }

    gw::ecs::World reload;
    (void)reload.component_registry().id_of<Pos>();
    (void)reload.component_registry().id_of<Vel>();
    (void)reload.component_registry().id_of<Health>();
    const auto lr = pw.load_slot(1, reload);
    std::uint64_t reload_hash = 0;
    if (lr == gw::persist::LoadError::Ok) {
        const auto rb = gw::ecs::save_world(reload, gw::ecs::SnapshotMode::PieSnapshot);
        std::vector<std::byte> rb_bytes(rb.size());
        std::memcpy(rb_bytes.data(), rb.data(), rb.size());
        reload_hash = gw::persist::ecs_blob_determinism_hash(std::span<const std::byte>(
            rb_bytes.data(), rb_bytes.size()));
    }

    meta.schema_version = gw::persist::kCurrentSaveSchemaVersion;
    if (pw.save_slot(3, world, meta) == gw::persist::SaveError::Ok) ++saves;

    std::printf("PERSIST OK — saves=%d migrations=%d cloud_rt=%d consent=CrashOnly events=%d "
                  "dsar_bytes=%llu reload_hash=%016llx reload_entities=%zu\n",
                  saves, migrations, cloud_rt, events,
                  static_cast<unsigned long long>(dsar_bytes),
                  static_cast<unsigned long long>(reload_hash),
                  reload.entity_count());

    const bool ok = (saves >= 3 && migrations >= 1 && cloud_rt >= 2 && events >= 500
                     && lr == gw::persist::LoadError::Ok && reload.entity_count() == 2048u);
    return ok ? 0 : 1;
}
