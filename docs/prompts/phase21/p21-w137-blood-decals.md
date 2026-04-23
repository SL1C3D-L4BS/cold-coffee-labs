# p21-w137-blood-decals - BloodDecalSystem — compute stamp + drip + 512-entry ring

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 137 |
| Tier | A |
| Depends on | `p21-w136-circle-post` |

## Inputs

- `engine/render/`

## Writes

- `engine/render/decals/blood_decal_system.hpp`
- `engine/render/decals/blood_decal_system.cpp`
- `engine/render/decals/decal_component.hpp`
- `shaders/blood_decal.hlsl`
- `tests/render/blood_decal_test.cpp`

## Exit criteria

- [ ] Ring-buffer capacity 512; oldest evicted FIFO
- [ ] Compute stamp + drip run between opaque and horror_post

## Prompt

New engine/render/decals/ module implementing a GPU-driven 512-entry
ring buffer of blood decals. DecalComponent attaches decals to
entities; compute pass stamps + drips.

