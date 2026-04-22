// engine/ecs/serialize.cpp
// ECS World <-> binary blob — ADR-0006 §2.2 / §2.9.
//
// Layout note: this implementation serializes entity-major rather than
// archetype-major (ADR §2.2 prescribes archetype-grouped chunk payloads).
// Per-entity component writes are still memcpy'd one-for-one, so the fast
// path stays a pure memcpy stream; the chunk-grouped layout is an
// optimization tracked against the PIE 10 ms budget. When profiling shows
// the entity-header overhead matters, switch to the grouped layout without
// changing the header or CRC semantics.

#include "serialize.hpp"

#include "archetype.hpp"
#include "component_registry.hpp"
#include "engine/core/serialization.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <unordered_map>

namespace gw::ecs {

using gw::core::BinaryReader;
using gw::core::BinaryWriter;
using gw::core::crc64_iso;

// Magic is 'GWS1' encoded as little-endian 'G','W','S','1' (0x31535747).
inline constexpr std::uint32_t kMagic          = 0x31535747U;
inline constexpr std::uint16_t kFormatVersion  = 1U;
inline constexpr std::uint16_t kFlagNone       = 0U;
inline constexpr std::size_t   kHeaderBytes    = 4 + 2 + 2 + 8 + 8;  // 24

std::string_view to_string(SerializationError error) noexcept {
    switch (error) {
        case SerializationError::BadMagic:                 return "bad magic";
        case SerializationError::UnsupportedVersion:       return "unsupported format_version";
        case SerializationError::CrcMismatch:              return "payload CRC mismatch";
        case SerializationError::Truncated:                return "truncated blob";
        case SerializationError::UnknownComponentType:     return "unknown component type";
        case SerializationError::ComponentSizeMismatch:    return "component size mismatch";
        case SerializationError::ComponentNotSerializable: return "component is not trivially copyable";
        case SerializationError::PayloadCorrupt:           return "payload structure invalid";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Payload writer — shared by save_world and save_entity.
// ---------------------------------------------------------------------------
namespace {

struct LocalComponentTable {
    // Maps live ComponentTypeId → local_id (u16) within this payload.
    std::unordered_map<ComponentTypeId, std::uint16_t> to_local;
    // Per local_id: infos in emit order.
    std::vector<ComponentTypeInfo> infos;
};

// Collect all live component types used by the given entities; assign local
// ids in stable-hash-sorted order so two payloads over equivalent worlds
// produce byte-identical streams (useful for golden-file regression tests).
LocalComponentTable build_component_table(const World&                        world,
                                           std::span<const Entity> entities) {
    const auto& reg = world.component_registry();
    std::vector<ComponentTypeId> seen;
    for (const Entity entity : entities) {
        const auto* slot = world.slot_for(entity);
        if (slot == nullptr || slot->archetype_id == kInvalidArchetypeId) {
            continue;
        }
        const auto& arch = world.archetype_table().archetype(slot->archetype_id);
        for (auto tid : arch.types()) {
            if (std::ranges::find(seen, tid) == seen.end()) {
                seen.push_back(tid);
            }
        }
    }
    std::ranges::sort(seen,
                       [&](ComponentTypeId lhs, ComponentTypeId rhs) {
                           return reg.info(lhs).stable_hash < reg.info(rhs).stable_hash;
                       });
    LocalComponentTable out;
    out.infos.reserve(seen.size());
    std::uint16_t next = 0;
    for (auto tid : seen) {
        out.to_local.emplace(tid, next++);
        out.infos.push_back(reg.info(tid));
    }
    return out;
}

// Returns true on success; false means a component marked for write was
// non-trivially-copyable (reflection path is a TODO for Phase 8).
bool write_payload(const World&                        world,
                   std::span<const Entity> entities,
                   std::vector<std::uint8_t>&            out) {
    const auto table = build_component_table(world, entities);

    BinaryWriter writer{out};
    writer.write<std::uint32_t>(static_cast<std::uint32_t>(entities.size()));
    writer.write<std::uint16_t>(static_cast<std::uint16_t>(table.infos.size()));

    // ComponentTable entries — ADR §2.2.
    for (std::size_t idx = 0; idx < table.infos.size(); ++idx) {
        const auto& info = table.infos[idx];
        writer.write<std::uint16_t>(static_cast<std::uint16_t>(idx));
        writer.write<std::uint64_t>(info.stable_hash);
        writer.write<std::uint32_t>(static_cast<std::uint32_t>(info.size));
        writer.write<std::uint8_t>(info.trivially_copyable ? 1U : 0U);
        writer.write_string(info.debug_name);
    }

    // Entities — one record per entity with inline component blobs.
    for (const Entity entity : entities) {
        writer.write<std::uint64_t>(entity.raw_bits());

        const auto* slot = world.slot_for(entity);
        if (slot == nullptr || slot->archetype_id == kInvalidArchetypeId) {
            writer.write<std::uint16_t>(0);   // component_count
            continue;
        }
        const auto& arch = world.archetype_table().archetype(slot->archetype_id);
        const auto& reg  = world.component_registry();

        writer.write<std::uint16_t>(static_cast<std::uint16_t>(arch.types().size()));
        for (auto tid : arch.types()) {
            const auto& info = reg.info(tid);
            if (!info.trivially_copyable) {
                return false;   // §2.5 pending
            }

            const auto local = table.to_local.at(tid);
            writer.write<std::uint16_t>(local);

            const std::byte* src =
                world.archetype_table().archetype(slot->archetype_id)
                    .component_ptr(reg, tid, slot->chunk_index, slot->slot_index);
            if (src == nullptr) {
                return false;
            }
            writer.write_bytes(src, info.size);
        }
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Payload reader — shared by load_world and load_entity.
// ---------------------------------------------------------------------------
namespace {

struct EntityRecord {
    std::uint64_t                                   bits = 0;
    std::vector<std::pair<std::uint16_t,            // local component id
                          std::vector<std::uint8_t>>> components;
};

struct ParsedPayload {
    // local_id -> live ComponentTypeId (kInvalidComponentTypeId if unknown)
    std::vector<ComponentTypeId>            local_to_live;
    std::vector<std::uint32_t>              local_sizes;     // from blob
    std::vector<std::uint8_t>               local_trivial;   // 0/1 flag
    std::vector<std::uint64_t>              local_stable_hash;
    // True iff the on-disk size for this local_id differs from the live
    // registry size — migration callback will be consulted in apply_payload.
    std::vector<std::uint8_t>               local_needs_migration;
    std::vector<EntityRecord>               entities;
};

// NOLINTBEGIN(readability-function-cognitive-complexity)
std::expected<ParsedPayload, SerializationError>
parse_payload(const World& world, BinaryReader& reader,
              const LoadOptions& opts) {
    ParsedPayload out;

    std::uint32_t entity_count = 0;
    std::uint16_t type_count   = 0;
    if (!reader.read(entity_count) || !reader.read(type_count)) {
        return std::unexpected(SerializationError::Truncated);
    }

    const auto& reg = world.component_registry();
    out.local_to_live.resize(type_count, kInvalidComponentTypeId);
    out.local_sizes.resize(type_count, 0);
    out.local_trivial.resize(type_count, 0);
    out.local_stable_hash.resize(type_count, 0);
    out.local_needs_migration.resize(type_count, 0);

    for (std::uint16_t type_idx = 0; type_idx < type_count; ++type_idx) {
        std::uint16_t local_id    = 0;
        std::uint64_t stable_hash = 0;
        std::uint32_t size        = 0;
        std::uint8_t  trivial     = 0;
        std::string   debug_name;
        if (!reader.read(local_id) || !reader.read(stable_hash) ||
            !reader.read(size)     || !reader.read(trivial)     ||
            !reader.read_string(debug_name)) {
            return std::unexpected(SerializationError::Truncated);
        }
        if (local_id >= type_count) {
            return std::unexpected(SerializationError::PayloadCorrupt);
        }

        if (auto live = reg.lookup_by_hash(stable_hash); live) {
            const auto& live_info = reg.info(*live);
            if ((trivial != static_cast<std::uint8_t>(0)) && live_info.size != size) {
                // Defer the decision: a migration callback may yet rescue
                // this payload in apply_payload. If no callback is set, the
                // applier will flag ComponentSizeMismatch.
                out.local_needs_migration[local_id] = 1;
            }
            out.local_to_live[local_id] = *live;
        } else if (opts.strict_unknown_components) {
            return std::unexpected(SerializationError::UnknownComponentType);
        }
        out.local_sizes[local_id]       = size;
        out.local_trivial[local_id]     = trivial;
        out.local_stable_hash[local_id] = stable_hash;
    }

    out.entities.resize(entity_count);
    for (std::uint32_t entity_idx = 0; entity_idx < entity_count; ++entity_idx) {
        auto& rec = out.entities[entity_idx];
        std::uint16_t comp_count = 0;
        if (!reader.read(rec.bits) || !reader.read(comp_count)) {
            return std::unexpected(SerializationError::Truncated);
        }
        rec.components.reserve(comp_count);

        for (std::uint16_t comp_idx = 0; comp_idx < comp_count; ++comp_idx) {
            std::uint16_t local_id = 0;
            if (!reader.read(local_id) || local_id >= type_count) {
                return std::unexpected(SerializationError::PayloadCorrupt);
            }
            if (out.local_trivial[local_id] == static_cast<std::uint8_t>(0)) {
                return std::unexpected(SerializationError::ComponentNotSerializable);
            }
            const auto byte_count = out.local_sizes[local_id];
            std::vector<std::uint8_t> bytes(byte_count);
            if (!reader.read_bytes(bytes.data(), byte_count)) {
                return std::unexpected(SerializationError::Truncated);
            }
            rec.components.emplace_back(local_id, std::move(bytes));
        }
    }
    return out;
}

std::expected<void, SerializationError>
apply_payload(World& world, const ParsedPayload& parsed, bool reuse_bits,
                const MigrateComponentFn& migrate) {
    const auto& reg = world.component_registry();
    for (const auto& rec : parsed.entities) {
        Entity entity = reuse_bits ? world.restore_entity_with_bits(rec.bits)
                                   : world.create_entity();
        if (entity.is_null()) {
            // Fall back to create_entity — rec.bits may collide when the
            // World wasn't truly empty or bits are invalid.
            entity = world.create_entity();
        }
        for (const auto& [local_id, bytes] : rec.components) {
            const auto tid = parsed.local_to_live[local_id];
            if (tid == kInvalidComponentTypeId) {
                continue;   // unknown + lenient
            }

            const bool needs_mig =
                local_id < parsed.local_needs_migration.size() &&
                parsed.local_needs_migration[local_id] != 0;

            if (needs_mig) {
                if (migrate == nullptr) {
                    return std::unexpected(SerializationError::ComponentSizeMismatch);
                }
                const auto& info = reg.info(tid);
                auto migrated = migrate(
                    parsed.local_stable_hash[local_id],
                    parsed.local_sizes[local_id],
                    static_cast<std::uint32_t>(info.size),
                    std::span<const std::uint8_t>{bytes});
                if (!migrated || migrated->size() != info.size) {
                    return std::unexpected(SerializationError::ComponentSizeMismatch);
                }
                if (!world.add_component_raw(entity, tid, migrated->data(), migrated->size())) {
                    return std::unexpected(SerializationError::ComponentSizeMismatch);
                }
            } else {
                if (!world.add_component_raw(entity, tid, bytes.data(), bytes.size())) {
                    return std::unexpected(SerializationError::ComponentSizeMismatch);
                }
            }
        }
    }
    return {};
}
// NOLINTEND(readability-function-cognitive-complexity)

} // namespace

// ---------------------------------------------------------------------------
// save_world / load_world — GWS1 header framing.
// ---------------------------------------------------------------------------

std::vector<std::uint8_t>
save_world(const World& world, SnapshotMode mode) {
    // Collect every live entity in stable visit order.
    std::vector<Entity> entities;
    entities.reserve(world.entity_count());
    world.visit_entities([&](Entity entity) { entities.push_back(entity); });

    std::vector<std::uint8_t> payload;
    payload.reserve(1024);
    if (!write_payload(world, entities, payload)) {
        return {};  // non-trivially-copyable component — caller bug (ADR §2.5 TODO)
    }

    if (mode == SnapshotMode::EntitySnapshot) {
        // No header for entity snapshots; caller owns framing.
        return payload;
    }

    std::vector<std::uint8_t> out;
    out.reserve(kHeaderBytes + payload.size());

    BinaryWriter writer{out};
    writer.write<std::uint32_t>(kMagic);
    writer.write<std::uint16_t>(kFormatVersion);
    writer.write<std::uint16_t>(kFlagNone);
    writer.write<std::uint64_t>(static_cast<std::uint64_t>(payload.size()));
    writer.write<std::uint64_t>(crc64_iso(payload));
    writer.write_bytes(payload.data(), payload.size());
    return out;
}

std::expected<void, SerializationError>
load_world(World& target_world, std::span<const std::uint8_t> blob, const LoadOptions& opts) {
    BinaryReader reader{blob};
    std::uint32_t magic   = 0;
    std::uint16_t version = 0;
    std::uint16_t flags   = 0;
    std::uint64_t bytes   = 0;
    std::uint64_t crc     = 0;
    if (!reader.read(magic) || !reader.read(version) || !reader.read(flags) ||
        !reader.read(bytes) || !reader.read(crc)) {
        return std::unexpected(SerializationError::Truncated);
    }
    if (magic != kMagic) {
        return std::unexpected(SerializationError::BadMagic);
    }
    if (version != kFormatVersion) {
        return std::unexpected(SerializationError::UnsupportedVersion);
    }

    auto payload = reader.take(static_cast<std::size_t>(bytes));
    if (!reader.ok()) {
        return std::unexpected(SerializationError::Truncated);
    }

    if (opts.verify_crc && crc64_iso(payload) != crc) {
        return std::unexpected(SerializationError::CrcMismatch);
    }

    BinaryReader payload_reader{payload};
    auto parsed = parse_payload(target_world, payload_reader, opts);
    if (!parsed) {
        return std::unexpected(parsed.error());
    }

    target_world.clear();
    return apply_payload(target_world, *parsed, /*reuse_bits=*/true, opts.migrate);
}

// ---------------------------------------------------------------------------
// save_entity / load_entity — headerless (ADR §2.6).
// ---------------------------------------------------------------------------

std::vector<std::uint8_t> save_entity(const World& world, Entity entity) {
    std::vector<std::uint8_t> payload;
    if (world.slot_for(entity) == nullptr) {
        return payload;
    }
    const std::array<Entity, 1> ents{{entity}};
    std::vector<std::uint8_t> out;
    write_payload(world, ents, out);
    return out;
}

std::expected<Entity, SerializationError>
load_entity(World& world, std::span<const std::uint8_t> blob) {
    BinaryReader reader{blob};
    auto parsed = parse_payload(world, reader, LoadOptions{});
    if (!parsed) {
        return std::unexpected(parsed.error());
    }
    if (parsed->entities.empty()) {
        return std::unexpected(SerializationError::PayloadCorrupt);
    }

    // Single-entity path always allocates a fresh handle — the destroyed
    // entity's old bits cannot be reused (the DestroyEntityCommand remaps
    // Entity-valued component fields externally, per ADR §2.6).
    const Entity created_entity = world.create_entity();
    for (const auto& [local_id, bytes] : parsed->entities.front().components) {
        const auto tid = parsed->local_to_live[local_id];
        if (tid == kInvalidComponentTypeId) {
            continue;
        }
        if (!world.add_component_raw(created_entity, tid, bytes.data(), bytes.size())) {
            return std::unexpected(SerializationError::ComponentSizeMismatch);
        }
    }
    return created_entity;
}

} // namespace gw::ecs
