# p28-w2-music-train - Symbolic music training pipeline

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 28 |
| Tier | A |
| Depends on | `p28-w1-wfc-rl` |

## Inputs

- `tools/cook/ai/music_symbolic_train/`

## Writes

- `tools/cook/ai/music_symbolic_train/`

## Exit criteria

- [ ] Trains the transformer used by Audio Weave

## Prompt

Python training loop producing .ggml checkpoints under
assets/ai/music/.

