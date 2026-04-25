# IBL and tonemapping system (Week 031)

target_sources(engine_render PRIVATE
    ibl.cpp
    ibl.hpp
)

# Include directories (project root is set on engine_render in render/CMakeLists.txt)
target_include_directories(engine_render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
