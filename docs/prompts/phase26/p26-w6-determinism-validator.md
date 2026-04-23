# p26-w6-determinism-validator - Determinism validator extension

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 26 |
| Tier | A |
| Depends on | `p26-w5-sandbox-director` |

## Inputs

- `tools/lint/check_determinism_diff.py`

## Writes

- `tools/lint/check_determinism_diff.py`

## Exit criteria

- [ ] Same seed + same checkpoint + same synthetic input = bitwise-identical Linux↔Windows

## Prompt

Extend nightly determinism.yml to exercise AI runtime paths.

