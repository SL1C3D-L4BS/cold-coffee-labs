# pre-eng-franchise-services - Reach franchise services from owners

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | `pre-eng-ai-runtime-dispatch` |

## Inputs

- `engine/services/`

## Writes

- `engine/render/CMakeLists.txt`
- `engine/world/CMakeLists.txt`
- `engine/audio/CMakeLists.txt`

## Exit criteria

- [ ] Each owning subsystem calls ServiceX::instance() during its init

## Prompt

Material Forge (render init), Level Architect (world init), Combat
Simulator (AI/combat init), Gore (damage init), Audio Weave (audio
init), Director (gameplay loop). One instance() call per owner; no
behaviour change.

