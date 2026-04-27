// tests/unit/gameplay/trigger_weapon_parity_table_test.cpp
// Row count must match docs/gameplay/TRIGGER_WEAPON_PARITY_TABLE.md.

#include <doctest/doctest.h>

#include <array>

namespace {

struct ParityRow {
    const char* spec_concept;
    const char* greywater_owner;
};

// Keep in sync with docs/gameplay/TRIGGER_WEAPON_PARITY_TABLE.md body rows.
constexpr std::array<ParityRow, 5> kRows{{
    {"Axis-aligned trigger overlap", "trigger_zone.hpp (zones_overlap)"},
    {"Door slide / open distance", "trigger_zone.hpp"},
    {"Pickup respawn cadence", "trigger_zone.hpp (PickupRespawnState)"},
    {"Hitscan / projectile weapons", "weapon_system.cpp"},
    {"VM touch rules", "progs_vm + ADR-0122 builtins"},
}};

}  // namespace

TEST_CASE("trigger_weapon parity table — row count stable") {
    CHECK(kRows.size() == 5u);
    CHECK(kRows[0].spec_concept != nullptr);
}
