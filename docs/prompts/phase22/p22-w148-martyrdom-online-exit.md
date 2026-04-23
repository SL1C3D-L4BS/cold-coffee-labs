# p22-w148-martyrdom-online-exit - sandbox_martyrdom_online — 2-peer exit gate

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `phase-week` |
| Phase | 22 |
| Week | 148 |
| Tier | A |
| Depends on | `p22-w147-grapple-blood-surf`, `pre-tc-bld-replay` |

## Inputs

- `engine/net/`
- `bld/bld-replay/`

## Writes

- `apps/sandbox_martyrdom_online/main.cpp`
- `apps/sandbox_martyrdom_online/CMakeLists.txt`
- `editor/panels/sacrilege/dialogue_graph_panel.cpp`
- `editor/panels/sacrilege/sin_signature_panel.cpp`
- `editor/panels/sacrilege/act_state_panel.cpp`
- `tests/net/martyrdom_determinism_test.cpp`
- `docs/02_ROADMAP_AND_OPERATIONS.md`

## Exit criteria

- [ ] 2-peer Steam-datagram session runs 5 min with no desync
- [ ] state hash every 60 ticks identical across peers
- [ ] editor panels (dialogue, sin-signature, act-state) land
- [ ] Phase 22 checkbox ticked in docs/02

## Prompt

Full Martyrdom loop across two clients. Steam datagram transport.
Hash state every 60 ticks; any divergence fails the test. Land the
three editor panels.

