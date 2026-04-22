# ADR 0068 — `.gwstr` binary localization tables

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16C)
- **Companions:** 0028 (`LocaleBridge`), 0069 (ICU runtime), 0070 (shaping).

## 1. Context

Phase 11 shipped `LocaleBridge` with in-memory FNV1a-hashed keys. Phase 16
promotes strings to an **on-disk binary format** so locales ship as
content-hashed, cook-time-validated, mmap-friendly blobs. This is the input
the engine reads at runtime; the source of truth remains XLIFF 2.1 (ADR-0069
§3) so translators never see a binary.

## 2. Format

```
.gwstr v1 layout (little-endian)

HeaderPrefix (32 bytes)
├── magic           u32  = 'GWSR'
├── version         u16  = 1
├── flags           u16  bit0 = bidi_hint_present
├── string_count    u32
├── blob_size       u32  (UTF-8 blob length)
├── locale_tag_len  u8   (BCP-47)
├── reserved        u8[3]
├── locale_tag      char[locale_tag_len]  // not inside header prefix

Index (string_count × 12 bytes)
├── key_hash        u32  (fnv1a32)
├── utf8_offset     u32  into blob
└── utf8_size       u32

Blob (UTF-8 bytes, no terminators)

Footer (36 bytes)
├── reserved        u32
└── blake3_256      u8[32]  content hash of [HeaderPrefix..Blob]
```

Index is sorted by `key_hash`; lookup is binary search. The blob is a
single concatenated UTF-8 pool; individual strings are `(utf8_offset,
utf8_size)`. Zero padding between strings; canonical byte-identical
output for deterministic cook.

## 3. Cook

`tools/cook/gw_cook` gains a `locale` stage: ingest `*.xliff` (ADR-0069
§3) → canonicalize → emit `content/locales/<tag>.gwstr`. The cook is
deterministic (same XLIFF → same bytes) and its BLAKE3 is embedded in
the VFS manifest.

## 4. Runtime load

`gw::i18n::load_gwstr(std::span<const std::byte>)` returns a
`StringTable` that the `LocaleBridge` can register as an override. The
loader is header-only read-path (no allocations after construction) and
returns `LocaleError::{Ok,BadMagic,VersionTooNew,Truncated,HashMismatch}`.

## 5. Compatibility & migration

- v1 is the floor. Any new field is a `v1 → v2` migration in
  `engine/i18n/gwstr_migration.cpp`.
- Strings loaded via `LocaleBridge::register_string_table(tag, table)`
  stack: the last table wins on collisions. This lets patches ship as
  small overlays without rebuilding the base locale.

## 6. Determinism

The cook stage hashes **after** NFC normalisation and **after** removing
trailing whitespace per translator convention. `bidi_hint_present` flips
when any string contains at least one RTL scalar (U+0590..U+06FF,
U+0700..U+08FF) — precomputed so the runtime doesn't re-scan.
