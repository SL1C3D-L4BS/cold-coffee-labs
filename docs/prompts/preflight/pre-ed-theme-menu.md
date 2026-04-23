# pre-ed-theme-menu - Theme registry boot + View → Theme menu

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `editor/theme/theme_registry.hpp`
- `editor/app/editor_app.cpp`

## Writes

- `editor/app/editor_app.cpp`

## Exit criteria

- [ ] Editor startup calls ThemeRegistry::instance().set_active(persisted_id) before apply_theme()
- [ ] View → Theme submenu swaps theme live; ImGui re-colour verified by a doctest
- [ ] CI green dev-win + dev-linux

## Prompt

Wire the editor theme system from docs/prompts/00_WIRE_UP_AUDIT.md
entry `pre-ed-theme-menu`.

1. Load `ThemeId` from the editor TOML config (see `editor/app/editor_app.cpp` apply_theme() site).
2. Call `gw::editor::theme::ThemeRegistry::instance().set_active(id)` before apply_theme().
3. Add a `View → Theme` submenu in the editor main-menu builder with
   Brewed Slate, Corrupted Relic, Field Test HC radio items. On click
   call `set_active()` and re-run `apply_theme()`.
4. Write a doctest asserting that after `set_active(CorruptedRelic)`
   `ImGui::GetStyle().Colors[ImGuiCol_WindowBg]` changes.

Follow ADR-0104 (three-profile theme). Do not break the persisted
layout ini.

