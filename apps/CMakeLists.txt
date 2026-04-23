# apps/ — Phase 11 Wave 11F and beyond.
#
# Each subdirectory is a small, self-contained executable that exercises
# a specific engine capability. Phase 11 introduces `sandbox_playable`,
# the *Playable Runtime* exit-demo (ADR-0029).

add_subdirectory(sandbox_playable)
add_subdirectory(sandbox_physics)
add_subdirectory(sandbox_living_scene)

# Phase 14 Wave 14L — Networking exit-demo (ADR-0055).
add_subdirectory(sandbox_netplay)

# Phase 15 — Persistence & telemetry exit-demo (ADR-0064).
add_subdirectory(sandbox_persistence)

# Phase 16 — Platform Services / i18n / a11y SHIP READY exit-demo (ADR-0073).
add_subdirectory(sandbox_platform_services)

# Phase 17 — Studio Renderer exit-demo (ADR-0082).
add_subdirectory(sandbox_studio_renderer)

# Phase 18 — Studio Ready (cinematics + mod + BLD seq surface).
add_subdirectory(sandbox_studio)

# Phase 19-C — Infinite Seed (Hell Seed Murmur3 / resource gate).
add_subdirectory(sandbox_infinite_seed)

# Phase 26 / Part B §12.7 — Standalone AI Director Sandbox (ADR-0107).
add_subdirectory(sandbox_director)

# Phase 22 / Part C §22 — Steam Deck Tier A smoke probe (ADR-0113).
add_subdirectory(sandbox_deck_probe)

# Phase 21 W142 — Hell Frame exit-gate sandbox (ADR-0108).
add_subdirectory(sandbox_hell_frame)
