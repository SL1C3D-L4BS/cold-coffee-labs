# assets/ui/ — Phase 11 Wave 11E .rml/.rcss template catalogue.
#
# Owned by ADR-0027. Loaded by UIService through the virtual filesystem
# bridge at boot (runtime) or by UIService unit tests (headless). Paths
# are stable — downstream code references them by literal filename.
#
# Files:
#   hud.rml       / hud.rcss        Sandbox HUD (health, shields, captions, FPS)
#   menu.rml      / menu.rcss       Root menu + shared menu-item style
#   settings.rml  / settings.rcss   User-facing settings (bound via SettingsBinder)
#   rebind.rml    / rebind.rcss     Controller/keyboard rebind pane
#   console.rml   / console.rcss    Dev console overlay (ADR-0025)
#
# Localisation:
#   Caption / menu strings are resolved through LocaleBridge::resolve().
#   Default locale table lives at assets/ui/strings.en-US.toml (Wave 11E).
