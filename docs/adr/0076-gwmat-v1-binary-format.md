# ADR 0076 — `.gwmat` v1 binary material file format

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 17 (Studio Renderer)
- **Wave:** 17B
- **Companions:** 0075 (MaterialWorld), 0068 (`.gwstr` binary tables).

## 1. Context

Material instances must round-trip deterministically across machines
(server authoritative → client), git (content-addressed assets) and
hot-reload. `.gwmat` v1 is the on-disk representation.

## 2. Decision — layout

```
+0   u32  magic        = 'GWMT' (0x474D5754 little-endian)
+4   u16  major        = 1
+6   u16  minor        = 0
+8   u32  flags        (bit0 = compressed)
+12  u32  payload_size (bytes from +32 to footer)
+16  u64  content_hash (FNV-1a-64 of [+0..+12] || parameter-block || slot-table)
+24  u32  param_block_size (≤ 256)
+28  u16  slot_count       (≤ 16)
+30  u16  template_name_len
+32  ... template_name (ASCII, no NUL, length prefix above)
     ... u32[64] parameter_block_layout_table  (key_hash, offset, type)
     ... u8[param_block_size] parameter_block
     ... slot_count × { u32 texture_fingerprint; u16 slot_idx; u16 reserved }
+end u32  footer_magic = 'EOGW' (0x45 4F 47 57)
```

- **Total header size: 32 bytes.**
- `parameter_block_layout_table` has up to 64 entries × 12 bytes = 768 B
  worst case. Packed with `SoA` to keep `memcpy` straightforward.
- `texture_fingerprint` is the 32-bit upper half of the texture's
  BLAKE3 id (ADR-0058 style).

## 3. Content hashing

- Algorithm: **FNV-1a-64** with a 64-bit Fibonacci diffusion finalizer.
  Identity hash, not security hash.
- Rationale (mirrors ADR-0068 §4): keeps `greywater::material` independent
  from `greywater::persist` + BLAKE3 to avoid the circular-dependency
  hazard that would arise if material I/O required `greywater::persist`.
- Determinism: identical input bytes produce identical hashes across
  Windows + Linux + clang-cl. Enforced by
  `phase17_gwmat_test.cpp::gwmat_roundtrip_is_bit_deterministic`.

## 4. Endianness, alignment, limits

- Little-endian only (all shipping targets; big-endian emission is a
  deliberate non-goal).
- 4-byte alignment on all integers; the 32-byte header + 256-byte
  payload keeps the whole record 4-byte aligned.
- Hard limits: ≤ 256 B parameter block, ≤ 16 texture slots, template
  name ≤ 128 B.

## 5. Forward / backward compatibility

- `major = 1` is frozen.
- Minor bump adds optional fields **after** the texture-slot table but
  **before** the footer. Unknown minor fields are skipped during load.

## 6. I/O API

```cpp
namespace gw::material {
  [[nodiscard]] GwMatBlob  encode_gwmat(const MaterialInstance&, const MaterialTemplate&);
  [[nodiscard]] std::expected<MaterialInstance, GwMatError>
                           decode_gwmat(std::span<const std::byte>, MaterialWorld&);
  [[nodiscard]] std::uint64_t content_hash(std::span<const std::byte>);
}
```

Errors: `GwMatError::MagicMismatch`, `VersionUnsupported`,
`HashMismatch`, `Truncated`, `TemplateUnknown`, `ParamLayoutMismatch`.

## 7. Test plan

`tests/unit/render/material/phase17_gwmat_test.cpp` — 10 cases:

1. encode/decode round-trip preserves every parameter.
2. `content_hash` is bit-deterministic across two builds.
3. Flipping one byte in the payload changes the hash.
4. Corrupted magic rejected.
5. Corrupted footer rejected.
6. Unknown template name rejected with `TemplateUnknown`.
7. Oversize parameter block (257 B) rejected.
8. Oversize slot count (17) rejected.
9. Minor-version forward-compat: v1.1 loaded by v1.0 code with a warning.
10. Hash stable when parameter block contains NaN/Inf (normalized to
    canonical bit-pattern before hashing).

## 8. Consequences

- **Positive.** Deterministic material content-hash, git-friendly,
  streaming-ready (compressed variant reserved for Phase 20).
- **Negative.** Version bumps require a minor bump + migration table —
  registered in `MaterialWorld::register_migration()`.
