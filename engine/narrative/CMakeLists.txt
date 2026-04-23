# engine/narrative/ — Phase 21/22 scaffold (ADR-0099, ADR-0100, ADR-0101, ADR-0103).
#
# Story-Bible runtime: skins the Martyrdom spine with Malakor / Niccolo, three
# Acts, Sin-signature driven voice director, and the Grace finale.
# Appended to greywater_core via FILE_SET HEADERS (monolithic pattern).

target_sources(greywater_core
    PRIVATE
        dialogue_graph.cpp
        act_state.cpp
        sin_signature.cpp
        voice_director.cpp
        grace_meter.cpp
        # pre-eng-narrative-tick: canonical five-module narrative step.
        narrative_tick_system.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            narrative_types.hpp
            dialogue_graph.hpp
            act_state.hpp
            sin_signature.hpp
            voice_director.hpp
            grace_meter.hpp
            narrative_tick_system.hpp
)
