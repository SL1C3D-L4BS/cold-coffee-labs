# pre-tc-tests-split - Integrate tests/split CMake modules behind GW_TESTS_SPLIT

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `tests/split/`
- `tests/CMakeLists.txt`

## Writes

- `tests/CMakeLists.txt`

## Exit criteria

- [ ] With GW_TESTS_SPLIT=ON, gw_tests_ai/physics/net build and ctest runs each

## Prompt

Add new `GW_TESTS_SPLIT` CMake option (default OFF). When ON,
include tests/split/gw_tests_{ai,physics,net}.cmake. Keep monolithic
gw_tests target for the legacy path.

