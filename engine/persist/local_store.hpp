#pragma once

#include "engine/persist/persist_types.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::persist {

struct SlotRow {
    SlotId        slot_id{0};
    std::string   display_name{};
    std::int64_t  unix_ms{0};
    std::int32_t  engine_ver{0};
    std::int32_t  schema_ver{0};
    std::int64_t  determinism_h{0};
    std::string   path{};
    std::int64_t  size_bytes{0};
    std::string   blake3_hex{};
};

class ILocalStore {
public:
    virtual ~ILocalStore() = default;

    virtual bool open_or_create() = 0;

    virtual bool upsert_slot(const SlotRow& row) = 0;
    virtual bool delete_slot(SlotId id)            = 0;
    virtual bool list_slots(std::vector<SlotRow>& out) = 0;
    virtual std::optional<SlotRow> get_slot(SlotId id) = 0;

    virtual bool set_setting(std::string_view ns, std::string_view key, std::string_view value) = 0;
    virtual std::optional<std::string> get_setting(std::string_view ns, std::string_view key)   = 0;

    virtual bool telemetry_enqueue(std::int64_t enqueued_ms, std::span<const std::byte> batch_id,
                                   std::span<const std::byte> payload, std::int32_t attempts,
                                   std::int64_t next_try_ms, std::span<const std::byte> blake3_128) = 0;
    virtual std::int64_t telemetry_pending_count() = 0;

    /// Deletes all rows from `telemetry_queue` (flush / tests / admin).
    virtual bool telemetry_delete_all_rows() = 0;

    /// Deletes rows where `enqueued_ms < cutoff_ms`. Returns the number of
    /// rows removed (ADR-0061 storage-minimisation: `tele.queue.max_age_days`).
    virtual std::int64_t telemetry_delete_older_than(std::int64_t cutoff_ms) = 0;

    /// Null PII fields + purge telemetry rows for user hash (ADR-0062).
    virtual bool wipe_pii_for_user(std::string_view user_id_hash_hex) = 0;

    virtual void shutdown() = 0;
};

[[nodiscard]] std::unique_ptr<ILocalStore> make_sqlite_local_store(std::string db_path);

} // namespace gw::persist
