# Cascaded shadow maps system (Week 030)

target_sources(engine_render PRIVATE
    cascaded_shadows.cpp
    cascaded_shadows.hpp
)

# Include directories (project root is set on engine_render in render/CMakeLists.txt)
target_include_directories(engine_render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
