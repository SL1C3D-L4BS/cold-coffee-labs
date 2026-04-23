# engine/render/post/ — Phase 17 Wave 17F (ADR-0079, 0080, 0081).

add_library(greywater_render_post STATIC
    bloom.cpp
    tonemap.cpp
    taa.cpp
    motion_blur.cpp
    dof.cpp
    chromatic.cpp
    grain.cpp
    post_world.cpp
    post_cvars.cpp
    # pre-eng-fsr2-hal-frame-graph wire-up (Part C §21 scaffolds).
    fsr2/fsr2_integration.cpp
    upscale_and_hdr_bridge.cpp
    ../hal/hdr_output.cpp
    ../hal/present_timing.cpp
)
add_library(greywater::render_post ALIAS greywater_render_post)

target_compile_features(greywater_render_post PUBLIC cxx_std_23)

target_include_directories(greywater_render_post
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_render_post
    PUBLIC
        greywater::core
)

target_sources(greywater_render_post
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            post_types.hpp
            bloom.hpp
            tonemap.hpp
            taa.hpp
            motion_blur.hpp
            dof.hpp
            chromatic.hpp
            grain.hpp
            post_world.hpp
            post_cvars.hpp
            fsr2/fsr2_integration.hpp
            upscale_and_hdr_bridge.hpp
            ../hal/hdr_output.hpp
            ../hal/present_timing.hpp
)
