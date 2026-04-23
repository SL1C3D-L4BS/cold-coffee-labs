# pre-eng-steam-input-glue - Plug Steam Input glue into input system

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `engine/input/steam_input_glue.hpp`

## Writes

- `engine/input/scanner.cpp`

## Exit criteria

- [ ] On Steam-enabled builds the scanner polls SteamInputSource each frame

## Prompt

In the input scanner, during init, query
`steam_input_glue::available()`; if true own a SteamInputSource and
poll it each frame alongside GLFW.

