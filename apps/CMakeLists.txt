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
