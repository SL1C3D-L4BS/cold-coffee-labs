# pre-ed-module-manifest - Call editor module-manifest boot

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | `pre-ed-sacrilege-panels`, `pre-ed-audit-panels` |

## Inputs

- `editor/modules/module_manifest.hpp`
- `editor/modules/modules_builtin.cpp`
- `editor/app/editor_app.cpp`

## Writes

- `editor/app/editor_app.cpp`

## Exit criteria

- [ ] register_builtin_modules() is called exactly once during editor startup

## Prompt

Invoke `gw::editor::modules::register_builtin_modules()` from the
EditorApplication constructor (before init_imgui). This primes the
manifest the editor will later drive panel construction from.

