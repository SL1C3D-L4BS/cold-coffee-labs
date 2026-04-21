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
#include <cstring>
#include <unordered_map>

namespace gw::ecs {

using gw::core::BinaryReader;
using gw::core::BinaryWriter;
using gw::core::crc64_iso;

// Magic is 'GWS1' encoded as little-endian 'G','W','S','1' (0x31535747).
inline constexpr std::uint32_t kMagic          = 0x31535747u;
inline constexpr std::uint16_t kFormatVersion  = 1u;
inline constexpr std::uint16_t kFlagNone       = 0u;
inline constexpr std::size_t   kHeaderBytes    = 4 + 2 + 2 + 8 + 8;  // 24

std::string_view to_string(SerializationError e) noexcept {
    switch (e) {
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
LocalComponentTable build_component_table(const World& w,
                                           std::span<const Entity> entities) {
    const auto& reg = w.component_registry();
    std::vector<ComponentTypeId> seen;
    for (const Entity e : entities) {
        const auto* slot = w.slot_for(e);
        if (!slot || slot->archetype_id == kInvalidArchetypeId) continue;
        const auto& arch = w.archetype_table().archetype(slot->archetype_id);
        for (auto tid : arch.types()) {
            if (std::find(seen.begin(), seen.end(), tid) == seen.end())
                seen.push_back(tid);
        }
    }
    std::sort(seen.begin(), seen.end(),
              [&](ComponentTypeId a, ComponentTypeId b) {
                  return reg.info(a).stable_hash < reg.info(b).stable_hash;
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
bool write_payload(const World& w,
                   std::span<const Entity> entities,
                   std::vector<std::uint8_t>& out) {
    const auto table = build_component_table(w, entities);

    BinaryWriter bw{out};
    bw.write<std::uint32_t>(static_cast<std::uint32_t>(entities.size()));
    bw.write<std::uint16_t>(static_cast<std::uint16_t>(table.infos.size()));

    // ComponentTable entries — ADR §2.2.
    for (std::uint16_t i = 0; i < table.infos.size(); ++i) {
        const auto& info = table.infos[i];
        bw.write<std::uint16_t>(i);
        bw.write<std::uint64_t>(info.stable_hash);
        bw.write<std::uint32_t>(static_cast<std::uint32_t>(info.size));
        bw.write<std::uint8_t>(info.trivially_copyable ? 1u : 0u);
        bw.write_string(info.debug_name);
    }

    // Entities — one record per entity with inline component blobs.
    for (const Entity e : entities) {
        bw.write<std::uint64_t>(e.bits);

        const auto* slot = w.slot_for(e);
        if (!slot || slot->archetype_id == kInvalidArchetypeId) {
            bw.write<std::uint16_t>(0);  // component_count
            continue;
        }
        const auto& arch = w.archetype_table().archetype(slot->archetype_id);
        const auto& reg  = w.component_registry();

        bw.write<std::uint16_t>(static_cast<std::uint16_t>(arch.types().size()));
        for (auto tid : arch.types()) {
            const auto& info = reg.info(tid);
            if (!info.trivially_copyable) return false;  // §2.5 pending

            const auto local = table.to_local.at(tid);
            bw.write<std::uint16_t>(local);

            const std::byte* src =
                w.archetype_table().archetype(slot->archetype_id)
                    .component_ptr(reg, tid, slot->chunk_index, slot->slot_index);
            if (!src) return false;
            bw.write_bytes(src, info.size);
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
    std::vector<EntityRecord>               entities;
};

std::expected<ParsedPayload, SerializationError>
parse_payload(const World& w, BinaryReader& br,
              const LoadOptions& opts) {
    ParsedPayload out;

    std::uint32_t entity_count = 0;
    std::uint16_t type_count   = 0;
    if (!br.read(entity_count) || !br.read(type_count)) {
        return std::unexpected(SerializationError::Truncated);
    }

    const auto& reg = w.component_registry();
    out.local_to_live.resize(type_count, kInvalidComponentTypeId);
    out.local_sizes.resize(type_count, 0);
    out.local_trivial.resize(type_count, 0);

    for (std::uint16_t i = 0; i < type_count; ++i) {
        std::uint16_t local_id    = 0;
        std::uint64_t stable_hash = 0;
        std::uint32_t size        = 0;
        std::uint8_t  trivial     = 0;
        std::string   debug_name;
        if (!br.read(local_id) || !br.read(stable_hash) ||
            !br.read(size)     || !br.read(trivial)     ||
            !br.read_string(debug_name)) {
            return std::unexpected(SerializationError::Truncated);
        }
        if (local_id >= type_count) {
            return std::unexpected(SerializationError::PayloadCorrupt);
        }

        if (auto live = reg.lookup_by_hash(stable_hash); live) {
            const auto& live_info = reg.info(*live);
            if (trivial && live_info.size != size) {
                return std::unexpected(SerializationError::ComponentSizeMismatch);
            }
            out.local_to_live[local_id] = *live;
        } else if (opts.strict_unknown_components) {
            return std::unexpected(SerializationError::UnknownComponentType);
        }
        out.local_sizes[local_id]   = size;
        out.local_trivial[local_id] = trivial;
    }

    out.entities.resize(entity_count);
    for (std::uint32_t i = 0; i < entity_count; ++i) {
        auto& rec = out.entities[i];
        std::uint16_t comp_count = 0;
        if (!br.read(rec.bits) || !br.read(comp_count)) {
            return std::unexpected(SerializationError::Truncated);
        }
        rec.components.reserve(comp_count);

        for (std::uint16_t c = 0; c < comp_count; ++c) {
            std::uint16_t local_id = 0;
            if (!br.read(local_id) || local_id >= type_count) {
                return std::unexpected(SerializationError::PayloadCorrupt);
            }
            if (!out.local_trivial[local_id]) {
                return std::unexpected(SerializationError::ComponentNotSerializable);
            }
            const auto sz = out.local_sizes[local_id];
            std::vector<std::uint8_t> bytes(sz);
            if (!br.read_bytes(bytes.data(), sz)) {
                return std::unexpected(SerializationError::Truncated);
            }
            rec.components.emplace_back(local_id, std::move(bytes));
        }
    }
    return out;
}

std::expected<void, SerializationError>
apply_payload(World& w, const ParsedPayload& parsed, bool reuse_bits) {
    for (const auto& rec : parsed.entities) {
        Entity e = reuse_bits ? w.restore_entity_with_bits(rec.bits)
                               : w.create_entity();
        if (e.is_null()) {
            // Fall back to create_entity — rec.bits may collide when the
            // World wasn't truly empty or bits are invalid.
            e = w.create_entity();
        }
        for (const auto& [local_id, bytes] : rec.components) {
            const auto tid = parsed.local_to_live[local_id];
            if (tid == kInvalidComponentTypeId) continue;   // unknown + lenient
            if (!w.add_component_raw(e, tid, bytes.data(), bytes.size())) {
                return std::unexpected(SerializationError::ComponentSizeMismatch);
            }
        }
    }
    return {};
}

} // namespace

// ---------------------------------------------------------------------------
// save_world / load_world — headered.
// ---------------------------------------------------------------------------

std::vector<std::uint8_t>
save_world(const World& w, SnapshotMode mode) {
    // Collect every live entity in stable visit order.
    std::vector<Entity> entities;
    entities.reserve(w.entity_count());
    w.visit_entities([&](Entity e) { entities.push_back(e); });

    std::vector<std::uint8_t> payload;
    payload.reserve(1024);
    if (!write_payload(w, entities, payload)) {
        return {};  // non-trivially-copyable component — caller bug (ADR §2.5 TODO)
    }

    if (mode == SnapshotMode::EntitySnapshot) {
        // No header for entity snapshots; caller owns framing.
        return payload;
    }

    std::vector<std::uint8_t> out;
    out.reserve(kHeaderBytes + payload.size());

    BinaryWriter bw{out};
    bw.write<std::uint32_t>(kMagic);
    bw.write<std::uint16_t>(kFormatVersion);
    bw.write<std::uint16_t>(kFlagNone);
    bw.write<std::uint64_t>(static_cast<std::uint64_t>(payload.size()));
    bw.write<std::uint64_t>(crc64_iso(payload));
    bw.write_bytes(payload.data(), payload.size());
    return out;
}

std::expected<void, SerializationError>
load_world(World& out, std::span<const std::uint8_t> blob, LoadOptions opts) {
    BinaryReader br{blob};
    std::uint32_t magic   = 0;
    std::uint16_t version = 0;
    std::uint16_t flags   = 0;
    std::uint64_t bytes   = 0;
    std::uint64_t crc     = 0;
    if (!br.read(magic) || !br.read(version) || !br.read(flags) ||
        !br.read(bytes) || !br.read(crc)) {
        return std::unexpected(SerializationError::Truncated);
    }
    if (magic   != kMagic)         return std::unexpected(SerializationError::BadMagic);
    if (version != kFormatVersion) return std::unexpected(SerializationError::UnsupportedVersion);

    auto payload = br.take(static_cast<std::size_t>(bytes));
    if (!br.ok()) return std::unexpected(SerializationError::Truncated);

    if (opts.verify_crc && crc64_iso(payload) != crc) {
        return std::unexpected(SerializationError::CrcMismatch);
    }

    BinaryReader pr{payload};
    auto parsed = parse_payload(out, pr, opts);
    if (!parsed) return std::unexpected(parsed.error());

    out.clear();
    return apply_payload(out, *parsed, /*reuse_bits=*/true);
}

// ---------------------------------------------------------------------------
// save_entity / load_entity — headerless (ADR §2.6).
// ---------------------------------------------------------------------------

std::vector<std::uint8_t> save_entity(const World& w, Entity e) {
    std::vector<std::uint8_t> payload;
    if (!w.slot_for(e)) return payload;
    const Entity ents[] = {e};
    std::vector<std::uint8_t> out;
    write_payload(w, ents, out);
    return out;
}

std::expected<Entity, SerializationError>
load_entity(World& w, std::span<const std::uint8_t> blob) {
    BinaryReader br{blob};
    auto parsed = parse_payload(w, br, LoadOptions{});
    if (!parsed) return std::unexpected(parsed.error());
    if (parsed->entities.empty()) {
        return std::unexpected(SerializationError::PayloadCorrupt);
    }

    // Single-entity path always allocates a fresh handle — the destroyed
    // entity's old bits cannot be reused (the DestroyEntityCommand remaps
    // Entity-valued component fields externally, per ADR §2.6).
    const Entity e = w.create_entity();
    for (const auto& [local_id, bytes] : parsed->entities.front().components) {
        const auto tid = parsed->local_to_live[local_id];
        if (tid == kInvalidComponentTypeId) continue;
        if (!w.add_component_raw(e, tid, bytes.data(), bytes.size())) {
            return std::unexpected(SerializationError::ComponentSizeMismatch);
        }
    }
    return e;
}

} // namespace gw::ecs
