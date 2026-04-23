# pre-tc-fuzz-preset - Add dev-fuzz-clang CMake preset

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `tests/fuzz/CMakeLists.txt`
- `CMakePresets.json`

## Writes

- `CMakePresets.json`

## Exit criteria

- [ ] cmake --preset dev-fuzz-clang configures and builds every fuzz harness
- [ ] .github/workflows/fuzz.yml succeeds

## Prompt

Add `dev-fuzz-clang` configurePreset (clang/clang-cl toolchain,
GW_FUZZ=ON, Address+UB sanitizers on). Update fuzz workflow to use
it.

