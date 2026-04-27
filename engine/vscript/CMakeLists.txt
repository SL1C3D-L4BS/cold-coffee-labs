# engine/vscript — visual scripting IR, parser, interpreter, bytecode VM,
# and cook-time integration. Text IR is authoritative; node graphs are views.

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
