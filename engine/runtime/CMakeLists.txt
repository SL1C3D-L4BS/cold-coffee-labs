# engine/runtime/ — progs VM host + future runtime services (ADR-0122).
target_sources(greywater_core
    PRIVATE
        progs_vm.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            progs_vm.hpp
)
