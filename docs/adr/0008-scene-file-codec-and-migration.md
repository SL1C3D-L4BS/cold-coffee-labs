# ADR 0008 — `.gwscene` file codec, atomic writes, and component schema migration

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 8 week 043)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the two-file stub at `engine/core/scene_format.{hpp,cpp}` (deleted in this commit; never shipped; conflicting magic number `GWSC` vs. ADR-0006's `GWS1`).
- **Superseded by:** —
- **Related:** ADR-0004 (ECS-World), ADR-0006 (ECS-World Serialization — the in-memory payload this ADR wraps for on-disk use), `CLAUDE.md` non-negotiables #8 (no `std::string_view` to C), #17 (float64 world positions — the first migration target), #19 (trait-driven sanity: "atomic writes or nothing"), `docs/05_ROADMAP_AND_MILESTONES.md §Phase 8 week 043`.

## 1. Context

ADR-0006 defined the **payload** shape for a world and the header framing (`GWS1 / format_version = 1`, CRC-64, optional zstd). It explicitly deferred two items to Phase 8:

1. The **file I/O wrapper** — the `engine/scene/scene_file.{hpp,cpp}` module that turns `std::vector<std::uint8_t>` (from `save_world`) into a durable `.gwscene` on disk, with atomic replace semantics.
2. The **component migration hook** — a registered per-type transformer that lets the load path accept an older on-disk component layout and massage it into the current layout. ADR-0006 §2.8 marked this out of scope; the first real need surfaces *immediately* in Phase 8 week 044 when `TransformComponent::position` widens from `glm::vec3` (12 B) to `glm::dvec3` (24 B, padded to 56 B total) to comply with CLAUDE.md #17.

A prior attempt (`engine/core/scene_format.{hpp,cpp}`) tried to do #1 directly in `engine/core`. It used a different magic number (`GWSC`), duplicated header logic, and never wired into the ECS serializer. Keeping it would fork the on-disk format. We delete it in this ADR.

## 2. Decision

### 2.1 Module layout

```
engine/scene/
  scene_file.hpp       // public API: save_scene / load_scene
  scene_file.cpp
  migration.hpp        // MigrationRegistry (per-component transformers)
  migration.cpp
  CMakeLists.txt       // wired from engine/CMakeLists.txt
```

The module links `greywater_core` (for `engine/core/serialization.hpp`) and `greywater::ecs` (for `engine/ecs/serialize.hpp`). No Vulkan / ImGui dependencies — scene I/O is pure data.

### 2.2 Public API

```cpp
namespace gw::scene {

enum class SceneIoError : std::uint8_t {
    OpenFailed,
    WriteFailed,
    ReadFailed,
    RenameFailed,
    DeserializeFailed,   // underlying ECS error (CRC, truncation, etc.)
    SerializeFailed,     // encoder rejected the live world
};

struct SaveOptions {
    bool zstd = true;
};

struct LoadSceneOptions {
    bool verify_crc = true;
    bool strict_unknown_components = false;
};

[[nodiscard]] std::expected<void, SceneIoError>
save_scene(const gw::ecs::World& w,
           const std::filesystem::path& path,
           SaveOptions opts = {});

[[nodiscard]] std::expected<void, SceneIoError>
load_scene(gw::ecs::World& out,
           const std::filesystem::path& path,
           LoadSceneOptions opts = {});

} // namespace gw::scene
```

`save_scene` and `load_scene` compose `gw::ecs::save_world(..., SnapshotMode::SceneFile)` + atomic file I/O + `gw::ecs::load_world`. Errors from the ECS layer map 1:1 onto `SceneIoError::{Serialize,Deserialize}Failed`; the original `SerializationError` is logged via `engine/core/log.hpp`.

### 2.3 Atomic writes

`save_scene` never writes directly to the target path. The sequence is:

1. Serialize the world to a heap buffer via `save_world`.
2. Write the buffer to `<path>.tmp-<pid>-<timestamp>` (same directory as target — same volume, required for rename atomicity on POSIX and Win32).
3. `fsync` the temp file on POSIX; `FlushFileBuffers` on Win32 (not strictly required by the ADR, but significantly reduces the "crash between write and rename" window).
4. Rename the temp file over the target: `rename(2)` on POSIX, `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` on Win32.
5. On any error the temp file is deleted and the target is left untouched.

This guarantees "either the old scene is intact, or the new scene is fully written and CRC-clean" — no half-written `.gwscene` ever reaches disk.

### 2.4 Component schema migration

The `MigrationRegistry` answers the question: *"the payload on disk says component X is 40 bytes, but the live `T` is 56 bytes — what do I do?"*

```cpp
namespace gw::scene {

// Migrates one component instance in place.
// src:  raw bytes of the on-disk component (size = old `size` from the payload's ComponentTable entry)
// dst:  live-sized buffer of `sizeof(T)` bytes that the loader will then
//        memcpy into the archetype chunk. The function owns filling `dst`
//        with a valid, trivially-copyable instance of T.
using MigrateFn = std::function<bool(
    std::span<const std::uint8_t> src,
    std::span<std::uint8_t>       dst)>;

struct MigrationKey {
    std::uint64_t stable_hash = 0;   // component stable_hash (the portable key)
    std::uint32_t from_size   = 0;   // payload-reported size that matched this entry
};

struct MigrationEntry {
    MigrateFn   fn;
    std::string reason;              // e.g. "TransformComponent: widen position vec3->dvec3"
};

class MigrationRegistry {
public:
    static MigrationRegistry& global();

    template <class T>
    void register_fn(std::uint32_t from_size, MigrateFn fn, std::string reason);

    [[nodiscard]] const MigrationEntry*
    find(std::uint64_t stable_hash, std::uint32_t from_size) const noexcept;
};

} // namespace gw::scene
```

The ECS loader (`gw::ecs::load_world`) is extended with an opt-in callback:

```cpp
struct LoadOptions {
    bool verify_crc                = true;
    bool strict_unknown_components = false;
    MigrateComponentFn migrate;    // NEW — called when payload.size != live.size
};
```

`scene_file.cpp` supplies a `migrate` that routes through `MigrationRegistry::global().find(...)`. When a migrator exists, `load_world` calls it per-instance; when it doesn't, `load_world` returns `ComponentSizeMismatch` unchanged.

`register_fn<T>(from_size, fn, reason)` is called once per process (`std::call_once` inside the module that owns `T`'s layout evolution). For editor-side components the registration happens inside `gw_editor_load_scene`'s first call (see `editor/bld_api/editor_bld_api.cpp::register_editor_migrations_once`).

#### 2.4.1 The TransformComponent case (week 044)

- v1 layout: `float position[3]` + `float rotation[4]` + `float scale[3]` = 40 bytes, trivially-copyable.
- v2 layout: `double position[3]` + `float rotation[4]` + `float scale[3]` + 4 B trailing padding = **56 bytes** (the `double` forces 8-byte alignment on the struct).

The registered migrator reads the v1 bytes with one `memcpy` each for `pos_f`, `quat_f`, `scale_f`, widens `pos_f` to `double` element-by-element, and writes a fresh v2 `TransformComponent` into `dst`. See `editor/bld_api/editor_bld_api.cpp::register_editor_migrations_once` for the full code; the migrator is ~30 lines and adds a `static_assert(sizeof(TransformComponent) == 56)` so a future struct change trips the build rather than silently corrupting scenes.

### 2.5 Why not handle migration one level up (in `scene_file.cpp`)?

Earlier draft: have `scene_file` pre-pass the payload, rewrite old-size component arrays in place to new-size arrays, hand a rewritten buffer to `load_world`. Rejected because:

- Requires re-parsing the ECS payload format in two places → ECS-layout knowledge leaks into scene-file code.
- A pre-pass cannot benefit from the ECS loader's per-entity context (it would have to rebuild the archetype table itself).
- Doubles the code surface that has to stay in sync with ADR-0006 §2.2.

The callback-in-LoadOptions version keeps migration logic *inside* the ECS loader, where the component table and archetype walk already exist, and keeps the registry as the only public surface that migration authors touch.

## 3. Consequences

### Immediate (this commit block — Phase 8 week 043)

- `engine/core/scene_format.{hpp,cpp}` deleted.
- `engine/scene/{scene_file,migration}.{hpp,cpp}` landed.
- `engine/ecs/serialize.{hpp,cpp}` gained `MigrateComponentFn` in `LoadOptions`; the `parse_payload` defers the `ComponentSizeMismatch` check to `apply_payload` where the callback is consulted.
- `editor/bld_api/editor_bld_api.cpp` registers the `TransformComponent` v1→v2 migration on first scene load; `gw_editor_save_scene` and `gw_editor_load_scene` go through the new module.
- Tests: `tests/unit/scene/scene_file_test.cpp` covers round-trip, CRC tamper detection, atomic rename, and an end-to-end migration scenario using a hand-crafted v1 payload.

### Short term (weeks 044-047)

- The transform system (week 044) relies on this migration to keep existing `.gwscene` assets loadable under the new `dvec3` shape without authoring-tool intervention.
- The VScript panel (week 047) saves `.gwvs` text + `.gwvs.layout.json` sidecars using plain `std::ofstream`; it does not go through `save_scene` because text IR is the ground-truth for scripts (CLAUDE.md #14).

### Long term

- Any future trivially-copyable component layout change registers a migrator with a new `from_size`. The registry is additive; multiple `from_size` entries for the same `stable_hash` coexist.
- If a component ever needs to change its `stable_hash` (e.g. namespace rename), a secondary "aliasing" table lands as an ADR-0008-follow-up. Out of scope here.
- `fsync` is currently best-effort; a dedicated ADR can define a stronger guarantee (fsync both the temp file *and* its parent directory on POSIX) if the content workflow demands it.

### Alternatives considered

1. **Write `.gwscene` directly to its final path via `std::ofstream`.** Rejected — any crash between `open(O_TRUNC)` and the last `write()` leaves a zero-byte or partial scene, which CRC-fails on load. The content-loss blast radius is unacceptable for authoring workflows.
2. **Store migrations as per-type `migrate_v1` / `migrate_v2` member functions.** Rejected — couples migration code to the component's own translation unit, which forces the component header to know about scene I/O. The registry pattern keeps components header-only and keeps migration code in the editor/tool TU that understands the evolution.
3. **Auto-generate migrations from reflection field diffs.** Rejected for Phase 8 — doable, but a `dvec3` ← `vec3` widening isn't a field add/remove, it's a type change, and the automatic path would still need hand-authored glue for that case. Field-rename / field-add auto-migration can land later under ADR-0006 §2.8's reflection path.

## 4. References

- ADR-0004 (ECS-World)
- ADR-0006 (ECS-World Serialization)
- `CLAUDE.md` non-negotiables #17 (float64 world-space), #19 (atomic writes)
- `docs/05_ROADMAP_AND_MILESTONES.md §Phase 8`
- POSIX rename(2) — <https://man7.org/linux/man-pages/man2/rename.2.html>
- Win32 `MoveFileExW` — <https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw>

---

*Drafted by Claude Opus 4.7 on 2026-04-21 as part of the Phase 8 fullstack push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*
