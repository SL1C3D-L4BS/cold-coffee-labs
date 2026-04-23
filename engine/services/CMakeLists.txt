# engine/services/ — Phase 27 scaffold (ADR-0102).
#
# Seven IP-agnostic franchise services. Each exposes a POD schema header in
# `<service>/schema/` and a thin shim `.cpp` that adapts the shipping engine
# subsystems (render/material, world, gameai, etc.) to the schema.

add_subdirectory(material_forge)
add_subdirectory(level_architect)
add_subdirectory(combat_simulator)
add_subdirectory(gore)
add_subdirectory(audio_weave)
add_subdirectory(director)
add_subdirectory(editor_copilot)
