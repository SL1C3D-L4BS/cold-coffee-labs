#pragma once

#include "engine/persist/persist_types.hpp"

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace gw::persist {

using SaveMigrationFn = std::function<bool(std::span<const std::byte> in, std::vector<std::byte>& out)>;

struct SaveMigrationStep {
    std::uint32_t      from_schema{0};
    std::uint32_t      to_schema{0};
    SaveMigrationFn    apply{};
};

/// Append-only registry (ADR-0057).
[[nodiscard]] std::span<const SaveMigrationStep> save_migration_steps() noexcept;

[[nodiscard]] LoadError run_save_migrations(std::uint32_t& in_out_schema, std::vector<std::byte>& container_bytes);

} // namespace gw::persist
