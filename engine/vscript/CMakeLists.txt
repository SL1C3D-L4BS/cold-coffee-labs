# engine/vscript — Visual Scripting IR + parser + interpreter (+ bytecode VM
# and cook pipeline in Wave 4). CLAUDE.md non-negotiable #14: text IR is the
# ground-truth; node graph is a projection.
# Phase 8 week 045 (ADR 0009 in docs/10_APPENDIX_ADRS_AND_REFERENCES.md).

target_sources(greywater_core
    PRIVATE
        ir.cpp
        parser.cpp
        interpreter.cpp
        bytecode.cpp
        vm.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            ir.hpp
            parser.hpp
            interpreter.hpp
            bytecode.hpp
            vm.hpp
)
