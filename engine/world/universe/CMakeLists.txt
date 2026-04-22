# Phase 19-A — Blacklake universe primitives (HEC + SDR + seed manager).

add_library(gw_universe STATIC
    hec.cpp
    sdr_noise.cpp
    universe_seed_manager.cpp
)
add_library(greywater::universe ALIAS gw_universe)

target_compile_features(gw_universe PUBLIC cxx_std_23)

target_include_directories(gw_universe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(gw_universe PUBLIC greywater::core)
