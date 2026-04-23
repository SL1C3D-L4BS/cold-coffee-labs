# p22-w147-grapple-blood-surf - Grapple + blood surf + Glory Kill + reality-warp post

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 22 |
| Week | 147 |
| Tier | A |
| Depends on | `p22-w146-player-controller` |

## Inputs

- `engine/render/post/`

## Writes

- `gameplay/movement/grapple.hpp`
- `gameplay/movement/grapple.cpp`
- `gameplay/movement/blood_surf.hpp`
- `gameplay/movement/blood_surf.cpp`
- `gameplay/combat/glory_kill.hpp`
- `gameplay/combat/glory_kill.cpp`
- `engine/render/post/reality_warp.hpp`
- `engine/render/post/reality_warp.cpp`
- `shaders/reality_warp.hlsl`

## Exit criteria

- [ ] Grapple hook projectile + tether + pull works
- [ ] Blood surf preserves momentum on decal surfaces
- [ ] Glory Kill triggers at enemy low-HP
- [ ] reality_warp post hooks fire on God Mode / Blasphemy State

## Prompt

Grapple hook with projectile + tether + pull. Blood surf preserves
momentum on surfaces carrying blood decals from W137. Glory Kill
triggers contextual finisher. reality_warp post stage renders
mirror-step distortion + gravity-invert during God Mode.

