# Milestone validation system (Week 032)

target_sources(engine_render PRIVATE
    milestone_validator.cpp
    milestone_validator.hpp
)

# Include directories (project root is set on engine_render in render/CMakeLists.txt)
target_include_directories(engine_render
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
