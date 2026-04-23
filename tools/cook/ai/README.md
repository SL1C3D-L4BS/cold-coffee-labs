# tools/cook/ai/ — Offline Cook-time AI Pipelines (Phase 28)

These pipelines are the **only** place ML training is allowed in the Greywater repo. They run offline, produce **pinned weights** (BLAKE3 + Ed25519), and deposit artefacts under `assets/ai/` after a human-in-the-loop review through `bld-governance`.

Governance: `docs/01` §2.2.1 (cook-time carve-out), ADR-0095, ADR-0097, ADR-0098.

## Pipelines

| Directory | Output | Runtime consumer |
|-----------|--------|------------------|
| `vae_weapons/` | VAE checkpoint → sampled weapon-variant pack | `gameplay/weapons/variants/` |
| `wfc_rl_scorer/` | RL-scored WFC tile-weight table per Circle profile | `engine/world/wfc/` at cook time |
| `music_symbolic_train/` | Tiny RoPE transformer weights (≤ 1M params) | `engine/ai_runtime/music_symbolic.hpp` |
| `neural_material/` | Neural material network weights | `engine/ai_runtime/material_eval.hpp` |
| `director_train/` | Director policy checkpoint (bounded-RL params) | `engine/ai_runtime/director_policy.hpp` |

## Discipline (binding)

1. Each pipeline has a `pipeline.yml` manifest listing seed, dataset manifest SHA, git commit SHA, hyperparameters, and the ADR that authorises runtime consumption.
2. Each pipeline emits weights + `weights.manifest.toml` (BLAKE3 + Ed25519 signature) alongside a human-readable report.
3. None of these pipelines run during per-PR CI. They run under `.github/workflows/ai_retrain.yml` (manual dispatch or weekly cron).
4. No pipeline may call an external LLM API. Self-hosted only.
5. Generated content is **frozen per release**. Changing weights is a `GoldenHashes` bump.
