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

# pre-eng-franchise-services wire-up: one entry point that references every
# franchise service so owners can link through a single facade until Phase 27
# lands per-owner direct dispatch.
target_sources(greywater_core
    PRIVATE
        franchise_services_facade.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            franchise_services_facade.hpp
)
