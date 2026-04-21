# engine/anim/ — Phase 13 (ADR-0039..0042).
#
# Deterministic skeletal animation (clip sample, blend tree, state machine,
# two-bone IK, FABRIK, CCD, morph, null-ragdoll), behind a PIMPL facade.
# The default backend is the null backend; the Ozz/ACL bridge is additionally
# linked when GW_ENABLE_OZZ / GW_ENABLE_ACL are ON (living-* preset).

add_library(greywater_anim STATIC
    skeleton.cpp
    pose.cpp
    clip.cpp
    blend_tree.cpp
    ik.cpp
    morph.cpp
    animation_world.cpp
    anim_cvars.cpp
    console_commands.cpp
)
add_library(greywater::anim ALIAS greywater_anim)

target_compile_features(greywater_anim PUBLIC cxx_std_23)

target_include_directories(greywater_anim
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_anim
    PUBLIC
        greywater::core
        greywater::console
        greywater::physics
        glm::glm
)

# Optional Ozz/ACL bridge — living-* preset enables GW_ENABLE_OZZ/GW_ENABLE_ACL.
if(GW_ENABLE_OZZ AND TARGET ozz_animation)
    target_compile_definitions(greywater_anim PUBLIC GW_ANIM_OZZ=1)
    target_link_libraries(greywater_anim PRIVATE ozz_animation)
    target_sources(greywater_anim PRIVATE impl/ozz_bridge.cpp)
endif()
if(GW_ENABLE_ACL AND TARGET acl)
    target_compile_definitions(greywater_anim PUBLIC GW_ANIM_ACL=1)
    target_link_libraries(greywater_anim PRIVATE acl)
    target_sources(greywater_anim PRIVATE impl/acl_decoder.cpp)
endif()

target_sources(greywater_anim
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            anim_types.hpp
            anim_config.hpp
            skeleton.hpp
            pose.hpp
            clip.hpp
            blend_tree.hpp
            ik.hpp
            morph.hpp
            events_animation.hpp
            animation_world.hpp
            anim_cvars.hpp
            console_commands.hpp
)
