# ADR 0006 — ECS World Serialization (binary, versioned; PIE snapshot format)

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the never-delivered Phase-3 week-016 serialization framework (`engine/core/serialization.cpp` remains a 10-line stub; see `docs/05 §Phase-3 amendment`).
- **Superseded by:** —
- **Related:** ADR-0004 (ECS-World — the thing being serialized); ADR-0005 (CommandStack — `DestroyEntityCommand` needs the single-entity snapshot path); `docs/AUDIT_MAP_2026-04-20.md §P1-11`; `CLAUDE.md` non-negotiables #5 (no raw `new`/`delete`), #8 (`std::string_view` never to C API), #17 (world-space positions are `float64`); `docs/02_SYSTEMS_AND_SIMULATIONS.md §Serialization`; `docs/03_VULKAN_RESEARCH_GUIDE.md` (indirectly — asset cooks use a related format).

## 1. Context

Three call sites need to serialize ECS state:

1. **PIE Enter-Play / Exit-Play snapshot.** Before entering play mode, the editor snapshots the entire `World`; after exiting play mode, it restores. This must be **fast** (ideally < 10 ms for a modest editor scene), **lossless**, and **allocation-light**.
2. **Scene file save/load** (Phase 8). A `.gwscene` file on disk carrying one world. Must be **forward-readable** (older-format file on newer engine → migrates), must be **stable enough for VCS diff** (commit-friendly; matters for Greywater's content workflow), must **survive component-struct evolution** (added fields, renamed fields).
3. **`DestroyEntityCommand` undo snapshot** (ADR-0005). Single-entity blob stored in the undo stack; used to re-add components on undo.

A single binary format serves all three. The PIE path is just the full-world variant with the fastest-possible codec. The scene-file path adds chunked write + integrity check. The command-snapshot path is the single-entity subtree with no container framing.

The Phase-3 stub (`engine/core/serialization.{hpp,cpp}`) was trying to be a generic per-type binary serializer keyed on `TypeInfo`. That direction was correct in spirit but never finished. We keep the buffer primitives and build on top.

## 2. Decision

### 2.1 Format family — `.gwblob` (in-memory) / `.gwscene` (on-disk)

A **single binary wire format** with three wrapping modes:

| Mode                | Header         | Where                                    | Compression   |
|---------------------|----------------|------------------------------------------|---------------|
| `pie_snapshot`      | `GWS1` + flags | heap buffer owned by editor              | off           |
| `scene_file`        | `GWS1` + flags | `.gwscene` file on disk                  | zstd optional |
| `entity_snapshot`   | *no header*    | inside an `ICommand` in the undo stack   | off           |

The payload is identical across modes; only the framing differs.

### 2.2 Wire shape (the payload)

All little-endian, all fields aligned to their natural size in the stream (no padding — we pay the price of `memcpy` into/out of a `uint64_t` scratch for misaligned reads, but reject the alternative of "pack then swap" as too easy to get wrong per platform).

```
Payload ::=
  u32        entity_count
  u16        archetype_count
  u16        component_type_count
  ComponentTable[component_type_count]
  ArchetypeTable[archetype_count]
  EntityTable[entity_count]
  Archetype.ChunkData[archetype_count]    (see §2.4)

ComponentTable entry ::=
  u16  component_type_id            (local to this file; maps into the live registry by stable_hash)
  u64  stable_hash                  (ComponentRegistry.stable_hash — the portable key)
  u32  size                         (sizeof(T) at serialize time; used to detect evolution)
  u8   trivially_copyable           (1 if memcpy fast path was used, 0 if reflection path)
  u16  debug_name_len               (≤ 255 in practice; u16 for alignment)
  u8[debug_name_len]  debug_name    (stable name string; for diagnostics only)

ArchetypeTable entry ::=
  u32  archetype_id                 (local to this file)
  u16  component_count
  u16  slots_per_chunk
  u16  chunk_count
  ComponentTypeId[component_count]  (the component set, sorted, local-to-this-file ids)

EntityTable entry ::=
  u64  entity_bits                  (pre-remap generation+index as written out)
  u32  archetype_id                 (local to this file)
  u16  chunk_index
  u16  slot_index

Archetype.ChunkData ::=
  for each chunk in archetype:
    u16  live_count
    // Then, for each component in archetype (in the archetype's stored order):
    //   if trivially_copyable:
    //     u8[live_count * component_size]  raw bytes
    //   else:
    //     ReflectionBlob (see §2.5)
```

- Local `component_type_id` and `archetype_id` values are only valid within the payload; remapping to the live `ComponentRegistry` happens at deserialize time via `stable_hash`. This keeps the format portable across builds where `ComponentRegistry` allocates ids in a different order.
- `entity_bits` is written as-is. On deserialize, we do **not** trust the generation — we rebuild the entity table from scratch. The `entity_bits` field is kept for *diagnostics* and for the single-entity snapshot case where we need to rewrite the new entity's handle into referring components (see §2.6).

### 2.3 Header (modes 1 and 2 only)

```
Header ::=
  u32  magic          'GWS1'  (0x31 0x53 0x57 0x47 on wire → bytes 'G','W','S','1')
  u16  format_version 1
  u16  flags          bit 0 = zstd compressed payload; bits 1..15 reserved
  u64  payload_bytes  (uncompressed size; used for the zstd decompress target buffer)
  u64  payload_crc64  (ISO CRC-64 of uncompressed payload; verify on load)
```

- `entity_snapshot` mode skips the header entirely — it's a blob living inside a command; the command knows the size.
- `pie_snapshot` mode keeps the header but sets `flags.zstd = 0`; the CRC is still computed so we catch editor-state corruption during PIE cycles.
- `scene_file` mode lets the caller choose `flags.zstd`. Default on.

### 2.4 The **fast path** — trivially-copyable components

For any component type where `trivially_copyable = true` (see ADR-0004 §2.4), the serializer emits `memcpy(out, chunk_soa_ptr, live_count * component_size)`. That's one instruction per component array per chunk. The deserializer does the inverse.

This is the PIE-critical path. A scene with 10⁴ entities of a single 32-byte archetype serializes in O(1) syscalls and ~320 KB of bulk `memcpy`. Well under the 10 ms budget.

### 2.5 The **slow path** — reflection (non-trivially-copyable components)

If a component has a non-trivial copy / move / destructor *or* has `ComponentRegistry.reflection != nullptr` and opted in, we serialize per-field using the reflection `TypeInfo`.

```
ReflectionBlob (for one component array in one chunk) ::=
  for each slot in [0, live_count):
    for each field in TypeInfo.fields:
      serialize_field(ctx, field, &component[slot])
```

`serialize_field` for built-in field types (integers, floats, `glm::vec*`, `glm::quat`, `std::string`, `Entity`) is hand-written. For nested reflected structs it recurses. For `std::vector<T>` / `std::array<T>` it writes `u32 count` + element payload. `std::string` is written as `u32 length + utf8_bytes`. **No `std::string_view` crosses the serialization boundary** (NN #8 — unrelated to C APIs here, but same principle: `string_view` is a non-owning lifetime, not a storage type).

**The deserializer tolerates evolution.** If a field is missing from the stream (deleted in newer code), default-construct it. If a field exists in the stream but not in the live type, skip. This requires the stream format to be self-describing per field, so:

```
Field ::=
  u32  field_name_hash     (fnv1a of field name)
  u16  wire_type           (enum: u8/u16/.../f32/f64/vec3/quat/string/entity/struct_begin/struct_end/array)
  u32  payload_bytes       (size of the payload that follows — lets us skip unknown fields)
  u8[payload_bytes]  payload
```

This is more bytes than the fast path but the reflection path is already slower; the per-field overhead (~10 bytes) is acceptable. Evolution-tolerant deserialization is the whole point of the reflection path — if you want max speed, make your component trivially-copyable.

### 2.6 Single-entity snapshot (for `DestroyEntityCommand` undo)

Same payload shape as §2.2 but with `entity_count = 1`, `archetype_count = 1`, and one chunk of `live_count = 1`. The archetype table and component table are cut to only what this entity uses.

On `undo()`, the command calls `World::restore_entity_from_blob(blob)` which:

1. Allocates a fresh `Entity` handle (new generation).
2. Creates the archetype if missing.
3. Populates the component values from the blob.
4. If any component field is of type `Entity` (e.g. `HierarchyComponent.parent`) pointing at the *destroyed* entity, the command's `undo()` uses its stored `{old_entity → new_entity}` map to remap. Today the only `Entity`-valued fields are in `HierarchyComponent`; the command remaps those explicitly.

Whole-world snapshot does not need cross-entity remapping because handles are written and read consistently within the same blob.

### 2.7 File framing for `.gwscene`

```
File ::=
  Header
  (flags.zstd ? zstd_compressed(Payload) : Payload)
```

- `.gwscene` files are not appended to; they're written atomically via temp-file-then-rename (win32 `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`, posix `rename`). No half-written scenes on crash.
- Zstd compression level: 3 (balance). Phase-8 can add a compressor pool if save time gets annoying.

### 2.8 Schema evolution rules

- **Adding a component type:** fine. Old files don't have it; new entities in new code just have it.
- **Removing a component type:** fine. Old files' entries for the dead type are skipped by id (no `stable_hash` match in registry).
- **Adding a field to a reflected component:** fine via §2.5 default-construct-missing.
- **Removing a field from a reflected component:** fine via §2.5 skip-unknown.
- **Renaming a field:** breaks field lookup via `field_name_hash`. A reflection-level annotation (`GW_RENAMED_FROM("old_name")`) can land in a later PR; out of scope here.
- **Changing a field's wire type:** the deserializer detects wire-type mismatch and default-constructs. Document this as "forbidden without migration code"; the engine logs a warning.
- **Changing a trivially-copyable component's layout:** this is the sharp corner. If the `size` field in the component table no longer matches the live type, the fast path cannot be used safely. Deserializer behavior: *if size differs, refuse to load the component array, default-construct the field in-place, log warning*. A migration hook (`gw_ecs_migrate_component<T>(old_bytes, out_new)`) can be registered per-type; out of scope for this ADR, documented as a Phase-8+ follow-up.
- **`format_version` bump (top-level wire shape changed):** deserializer refuses to load versions it doesn't know; a migrator is a separate PR. `GWS1 / format_version = 1` is what we ship now.

### 2.9 Code layout

- `engine/core/serialization.{hpp,cpp}` — **kept** as the low-level buffer primitives (`BinaryWriter`, `BinaryReader`, primitive `write<T>`, `read<T>` for arithmetic, string, vector). The existing 10-line stub is replaced with real code here.
- `engine/ecs/serialize.{hpp,cpp}` — the ECS-specific encoder/decoder: walks `World` to produce a payload, consumes a payload to rebuild a `World`. Public surface:

```cpp
namespace gw::ecs {

enum class SnapshotMode : std::uint8_t {
    PieSnapshot,      // header, no compression
    SceneFile,        // header, optional compression
    EntitySnapshot,   // no header
};

[[nodiscard]] std::vector<std::uint8_t>
save_world(const World&, SnapshotMode, bool zstd_if_scene = true);

struct LoadOptions {
    bool  verify_crc = true;
    // Fail (return unexpected) vs. best-effort (log + continue) when a
    // component type in the payload isn't registered in the live World.
    bool  strict_unknown_components = false;
};

[[nodiscard]] std::expected<void, SerializationError>
load_world(World& out, std::span<const std::uint8_t>, LoadOptions = {});

// Single-entity paths used by DestroyEntityCommand (ADR-0005).
[[nodiscard]] std::vector<std::uint8_t> save_entity(const World&, Entity);
[[nodiscard]] std::expected<Entity, SerializationError>
load_entity(World&, std::span<const std::uint8_t>);

} // namespace gw::ecs
```

### 2.10 Error model

`SerializationError` is an enum + short `std::string_view` message (owned by static storage). Cases:

- `BadMagic`, `UnsupportedVersion`, `CrcMismatch`, `Truncated` (header issues)
- `UnknownComponentType` (only if `strict_unknown_components`)
- `ComponentSizeMismatch` (fast-path, no migrator)
- `FieldTypeMismatch` (reflection path; downgraded to warning otherwise)
- `ZstdDecompressFailed`

The `std::expected<void, SerializationError>` return is consistent with the rest of the engine (see `engine/assets/asset_error.hpp`).

## 3. Consequences

### Immediate (this commit block — under Phase-7 Path-A)

- Existing 10-line stub in `engine/core/serialization.cpp` is replaced with real `BinaryWriter`/`BinaryReader`.
- `engine/ecs/serialize.{hpp,cpp}` lands.
- Tests land under `tests/unit/ecs/serialize_test.cpp`: trivial-only round-trip, reflection round-trip, evolution cases (added field / removed field), single-entity snapshot round-trip, CRC failure detection.
- `DestroyEntityCommand` (ADR-0005) wires to `save_entity` / `load_entity`.
- PIE `EnterPlay` calls `save_world(..., PieSnapshot)` into an editor-owned buffer; `ExitPlay` calls `load_world` to restore.

### Short term (Phase 8)

- `.gwscene` file I/O (`save_world(..., SceneFile)` + `std::ofstream`/`std::ifstream` wrappers). Probably `engine/assets/scene_file.{hpp,cpp}` or similar.
- Scene-file asset entry in the cook manifest, same way mesh/texture/shader assets currently flow.
- The editor's File menu grows "Open Scene" / "Save Scene" / "Save Scene As" wired to this.

### Long term

- Migration hook API (`gw_ecs_migrate_component<T>`) if any trivially-copyable component layout ever evolves.
- Field-rename annotation on reflection (`GW_RENAMED_FROM`).
- Zstd dictionary training for scene files if compression ratio matters to the content workflow.
- Cross-platform determinism audit — floats serialized round-trip losslessly (they already do, binary memcpy) but any text-forms we introduce later must be reviewed.

### Alternatives considered

1. **FlatBuffers / Cap'n Proto / Protobuf.** Rejected — all three add an IDL generation step, which conflicts with our "reflection is the schema" design. Also: FlatBuffers' zero-copy access is nice for read-heavy asset formats but here we're always deserializing into live `World` state anyway.

2. **JSON / TOML.** Rejected for PIE-snapshot path (way too slow, way too large). For `.gwscene` it has commit-diff appeal but binary + zstd with a stable-order archetype walk is VCS-diff-tolerable (`git diff` won't show useful hunks, but `.gwscene.meta` companion files can — Phase-8 concern). Ship binary now.

3. **Per-component `serialize()` / `deserialize()` free functions.** Rejected — that's what Phase-3's stub was reaching for. Reflection already provides the field list; hand-writing per-component serializers duplicates that work. The reflection path *is* the per-component serializer.

4. **Store `Entity` references by stable GUID rather than `entity_bits`.** Rejected as over-engineered for the PIE snapshot use case (round-trip same process, handles are internally consistent). Relevant if we ever cross-reference entities across separate `.gwscene` files (prefabs?), at which point a dedicated ADR handles the external-reference surface.

5. **Single-archetype + per-field stream only; no trivially-copyable fast path.** Rejected — the 10⁴-entity budget requires the fast path. Trivially-copyable is common enough (basic math components, numeric state) that making it the default is correct.

## 4. References

- ADR-0004 (ECS-World)
- ADR-0005 (Editor CommandStack — `DestroyEntityCommand`)
- `docs/AUDIT_MAP_2026-04-20.md §P1-11`
- `docs/02_SYSTEMS_AND_SIMULATIONS.md §Serialization`
- ISO CRC-64 specification — [ISO 3309](https://www.iso.org/standard/17151.html)
- Zstandard reference — [zstd.github.io](https://github.com/facebook/zstd)
- FlatBuffers (reference for why-not) — [google.github.io/flatbuffers](https://google.github.io/flatbuffers/)

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*
