# p21-w138-circle-skins - Circle fog params + 9 Location Skins

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 21 |
| Week | 138 |
| Tier | A |
| Depends on | `p21-w137-blood-decals` |

## Inputs

- `engine/world/circles/`

## Writes

- `engine/world/circles/circle_profile.hpp`
- `engine/world/circles/location_skins.cpp`

## Exit criteria

- [ ] CircleProfile::fog_params drives in-engine fog density/color/height
- [ ] 9 Location Skins (Vatican Necropolis, Colosseum of Echoes, Sewer of Grace, Hell's Factory, Rome Above, Palazzo of the Unspoken, Open Arena, Heaven's Back Door, Silentium) exposed as data entries

## Prompt

Extend CircleProfile with fog_params + location_skin. Populate nine
skins with skybox + albedo tint + audio-bed refs per Story Bible.

