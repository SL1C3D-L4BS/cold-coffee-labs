# engine/scene/seq — .gwseq sequencer + timeline ECS (Phase 18-A).
#
# The requested `gw_render_hal` target name maps to `engine_render` in this
# repository (Vulkan HAL + shader stack); linking it keeps ABI parity with
# other engine modules that already depend on the HAL — see
# docs/10_APPENDIX_ADRS_AND_REFERENCES.md.

add_library(gw_seq STATIC
    gwseq_codec.cpp
    sequencer_world.cpp
    seq_camera.cpp
    seq_spline.cpp
    seq_cut.cpp
)

target_include_directories(gw_seq PUBLIC "${CMAKE_SOURCE_DIR}")

target_link_libraries(gw_seq
    PUBLIC
        greywater::core
        greywater::assets
    PRIVATE
        engine_render
)

if(TARGET glm::glm)
    target_link_libraries(gw_seq PUBLIC glm::glm)
endif()
