# ADR 0040 — ACL integration + `.kanim` compressed clip format

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 13 Wave 13A)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0039

## 1. Context

Ozz 0.16.0 ships with its own (lightweight) KeyframeReducer and non-uniformly-sampled track compression. It is **not** bit-identical across compiler upgrades when FMA reordering kicks in. ACL 2.1.0 (last tag 2023-12-07; `develop` tracked through 2025-11 commits) provides a deterministic, compiler-stable, uniformly-sampled decoder that is widely used in shipped titles (Unreal, Lumberyard).

Binding context:
- #3 header quarantine — `<acl/**>` lives only in `engine/anim/impl/acl_decoder.{hpp,cpp}`
- Phase-12 determinism ladder requires bit-identical clip bytes across Win + Linux
- Phase-15 persistence (ADR-0096 forward-ref) will content-hash `.kanim` blobs

## 2. Decision

### 2.1 Decoder family — **uniformly-sampled only**

ACL supports uniformly-sampled and non-uniformly-sampled track compression. We ship **only** uniformly-sampled decoders. Rationale:
- Uniform compression is bit-deterministic under clang 17+ with `-fno-fast-math`.
- Non-uniform decoding dispatches on sample density and introduces FMA-ordering drift.
- Memory savings of non-uniform over uniform are <5% for locomotion clips (Greywater's typical content).

`engine/anim/impl/acl_decoder.cpp` calls `acl::decompress_tracks` with `acl::decompression_settings_uniform` only. Non-uniform settings are `#error`'d at compile time.

### 2.2 `.kanim` on-disk layout

`.kanim` is content-addressed via xxHash3-128 (matches Phase-6 asset convention).

```
offset  size   field
0x00    4      'KANM'   magic
0x04    4      u32      version (current = 1)
0x08    8      u64      source_gltf_xxh3_lo
0x10    8      u64      source_gltf_xxh3_hi
0x18    4      u32      flags (bit0=has_root_motion, bit1=additive_layer)
0x1C    4      u32      ozz_skeleton_ref_hash (FNV-1a-32 of skeleton joint names)
0x20    4      u32      duration_us (microseconds)
0x24    4      u32      track_count
0x28    4      u32      acl_blob_bytes
0x2C    4      u32      reserved (zero)
0x30    *      bytes    ACL compressed track payload (aligned 16)
```

All multi-byte integers are little-endian. `acl::compressed_tracks` is embedded verbatim starting at `0x30`.

### 2.3 Cook pipeline

```
source.gltf
    → fastgltf::Parser           (Phase 6, already pinned)
    → ozz::animation::offline::RawAnimation (Ozz offline — tools path, not linked into engine)
    → acl::AnimationClip          (offline conversion; ozz-track → acl-track)
    → acl::compress_tracks        (uniform settings; deterministic)
    → .kanim on-disk blob
```

`tools/cook/kanim_cli` is the headless CLI; `engine/anim/assets/kanim_cooker.{hpp,cpp}` is the library. Both are guarded by `GW_ENABLE_ACL` — clean-checkout CI builds a stub cooker that writes a canonical-zero `.kanim` (all-zero track data, valid header).

### 2.4 Runtime load path

`engine/anim/assets/kanim_loader.cpp` maps the file, validates magic + version, asserts the skeleton ref-hash matches the attached skeleton, and exposes a raw span to the `acl_decoder` bridge. No heap allocations beyond Ozz's internal sampler buffers (pooled).

### 2.5 Content-hash determinism gate

CI job `kanim_cook_parity`:
1. Cook the same `.gltf` on Windows + Linux under `living-*` preset.
2. xxHash3-128 over `.kanim` bytes must match.
3. ACL pin bump reruns this gate.

## 3. Consequences

- Memory saving over raw Ozz compression: 10-15% typical locomotion; 20-25% on long-duration clips (graze loops, Phase 23).
- Decoder CPU: +5-8% vs Ozz native; acceptable for <2 ms animation-step budget.
- Upgrade procedure: ACL bump → PR with freshly-cooked corpus + `kanim_cook_parity` green across both OSes.

## 4. Alternatives considered

- **Ozz native compression only** — rejected: compiler-stability risk, Phase-12 cross-OS replay is already fragile under FMA reordering.
- **GLTF runtime format** — rejected: unacceptable memory + load-time cost for 100-character scenes.
- **Custom in-house codec** — rejected: out of scope; ACL is a known quantity.
