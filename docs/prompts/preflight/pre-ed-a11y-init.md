# pre-ed-a11y-init - Init editor a11y + Accessibility menu

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `editor/a11y/editor_a11y.hpp`
- `editor/a11y/editor_a11y.cpp`

## Writes

- `editor/app/editor_app.cpp`

## Exit criteria

- [ ] apply(EditorA11yConfig) runs after apply_theme()
- [ ] Window → Accessibility opens a modal with every toggle

## Prompt

Wire editor accessibility config per ADR-0117.
- Own an `EditorA11yConfig` member on EditorApplication.
- Call `gw::editor::a11y::apply(cfg)` after `apply_theme()`.
- Add `Window → Accessibility…` main-menu entry opening a modal
  panel that mutates cfg and calls apply() on change.

