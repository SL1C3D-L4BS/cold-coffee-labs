# `tests/fuzz/` — LibFuzzer / OSS-Fuzz harnesses

Part C §25 scaffold (ADR-0117). Nightly fuzz coverage across the highest-risk
input surfaces. All harnesses are:

- Deterministic (seeded, no wall-clock, no filesystem mutation).
- Clang-only (`-fsanitize=fuzzer,address,undefined`).
- Reproducible via `build/fuzz-<target>` presets.

## Harnesses

| Harness | Target | Source |
|---|---|---|
| `fuzz_gwscene` | `.gwscene` loader | `fuzz_gwscene.cpp` |
| `fuzz_bld_ipc` | BLD C-ABI message dispatch | `fuzz_bld_ipc.cpp` |
| `fuzz_mod_manifest` | Mod manifest JSON schema | `fuzz_mod_manifest.cpp` |
| `fuzz_replay` | Gameplay replay schema | `fuzz_replay.cpp` |
| `fuzz_dialogue_graph` | Narrative dialogue graph parser | `fuzz_dialogue_graph.cpp` |

## Corpora

Seed corpora live under `tests/fuzz/corpus/<harness>/`. Additions from crash
triage should be minimized via `cmin` and committed.

## Running locally

```bash
cmake --preset fuzz
cmake --build --preset fuzz
./build/fuzz/bin/fuzz_gwscene -max_total_time=300 tests/fuzz/corpus/fuzz_gwscene/
```

## CI

`.github/workflows/fuzz.yml` runs each harness for 30 minutes nightly. Crashes
post to GitHub issues via the existing triage bot (ADR-0117).
