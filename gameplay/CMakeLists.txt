# gameplay/ — the hot-reloadable C++ dynamic library.
# docs/01_CONSTITUTION_AND_PROGRAM.md §2.5: only the gameplay module hot-reloads; engine core and BLD do not.
# Phase 1 provides a loadable skeleton; Phase 2 wires the platform-layer loader
# and file-watcher (engine/scripting/).

add_library(greywater_gameplay SHARED
    gameplay.cpp
    gameplay_dll_linkage.cpp
    simulation_tick.cpp
    # Phase 23 — Sacrilege enemy archetypes (BT + combat sim).
    enemies/enemy_bt.cpp
    # Phase 22 — Martyrdom skin: God Mode / Grace / Blasphemy-state side channels.
    martyrdom/grace_meter.cpp
    # Phase 21/22/23 — three-Act ticks (Story Bible — docs/11).
    acts/i/act_i_rite_of_unspeaking.cpp
    acts/ii/act_ii_blasphemy_circuit.cpp
    acts/iii/act_iii_unmaking.cpp
    # Phase 23 — Logos dual-ending selector (docs/07 §7b, ADR-0101).
    boss/logos/phase4_selector.cpp
    boss/god_machine/god_machine.cpp
    # Part C §24 — Gameplay a11y WCAG 2.2 AA (ADR-0116).
    a11y/gameplay_a11y.cpp
    # Phase 21 W140 — Diegetic HUD model (ADR-0108).
    hud/diegetic_hud.cpp
    # Phase 22 W146 — swept-AABB player controller (docs/07 §3).
    movement/player_controller.cpp
    # Phase 22 W147 — grapple / blood surf / glory kill.
    movement/grapple.cpp
    movement/blood_surf.cpp
    combat/glory_kill.cpp
    combat/gore_system.cpp
    combat/encounter_director_bridge.cpp
    weapons/weapon_system.cpp
)
target_link_libraries(greywater_gameplay PRIVATE
    greywater::core
    greywater::gameai
)

# Export symbols for the loader on Windows without a .def file.
set_target_properties(greywater_gameplay PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PREFIX ""  # greywater_gameplay.dll / greywater_gameplay.so
)

add_subdirectory(mod_example)
