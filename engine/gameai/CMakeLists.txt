# engine/gameai/ — Phase 13 (ADR-0043..0046).
#
# Combined Behavior-Tree executor + Navmesh tile bake + agent path-follow,
# behind a PIMPL facade. The default backend is the null backend; the
# Recast/Detour bridge is additionally linked when GW_ENABLE_RECAST is ON
# (living-* preset).

add_library(greywater_gameai STATIC
    blackboard.cpp
    behavior_tree.cpp
    gameai_world.cpp
    gameai_cvars.cpp
    console_commands.cpp
)
add_library(greywater::gameai ALIAS greywater_gameai)

target_compile_features(greywater_gameai PUBLIC cxx_std_23)

target_include_directories(greywater_gameai
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_gameai
    PUBLIC
        greywater::core
        greywater::console
        greywater::physics
        glm::glm
)

if(GW_ENABLE_RECAST AND TARGET Recast)
    target_compile_definitions(greywater_gameai PUBLIC GW_AI_RECAST=1)
    target_link_libraries(greywater_gameai PRIVATE Recast Detour DetourCrowd)
    target_sources(greywater_gameai PRIVATE impl/recast_bridge.cpp)
endif()

target_sources(greywater_gameai
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            gameai_types.hpp
            gameai_config.hpp
            blackboard.hpp
            behavior_tree.hpp
            navmesh.hpp
            events_gameai.hpp
            gameai_world.hpp
            gameai_cvars.hpp
            console_commands.hpp
)
