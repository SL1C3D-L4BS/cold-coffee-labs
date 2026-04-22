# Scene module — .gwscene file I/O + component schema migration registry.
# Phase 8 week 043 (ADRs 0006 + 0008 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md).

target_sources(greywater_core
    PRIVATE
        scene_file.cpp
        migration.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            scene_file.hpp
            migration.hpp
)
