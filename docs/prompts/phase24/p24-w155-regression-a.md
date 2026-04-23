# p24-w155-regression-a - Regression gate: all 711+ tests + sanitizers + fuzz + determinism

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 24 |
| Week | 155 |
| Tier | A |
| Depends on | `p23-w154-god-machine-rc` |

## Inputs

- `tests/`
- `.github/workflows/`

## Writes

- `.github/workflows/ci.yml`

## Exit criteria

- [ ] All tests green on dev-win
- [ ] dev-linux
- [ ] dev-fuzz-clang
- [ ] dev-asan
- [ ] dev-ubsan

## Prompt

Zero-fail regression over the full matrix.

