// engine/persist/migration.cpp — save migration registry (ADR-0057).

#include "engine/persist/migration.hpp"

namespace gw::persist {

namespace {

// Append-only chain. Phase-15F ships a header-level bump in `PersistWorld::migrate_slot`;
// byte-level transforms register here in future revisions.
const SaveMigrationStep kSteps[] = {
    {1, 1,
     [](std::span<const std::byte> in, std::vector<std::byte>& out) -> bool {
         out.assign(in.begin(), in.end());
         return true;
     }},
};

} // namespace

std::span<const SaveMigrationStep> save_migration_steps() noexcept {
    return std::span<const SaveMigrationStep>(kSteps);
}

LoadError run_save_migrations(std::uint32_t& in_out_schema, std::vector<std::byte>& container_bytes) {
    (void)container_bytes;
    // Phase 15: on-disk migrations for `.gwsave` are handled by re-serializing via
    // `PersistWorld::migrate_slot` (header `schema_version` bump + footer recompute).
    (void)in_out_schema;
    return LoadError::Ok;
}

} // namespace gw::persist
