# Core library

target_sources(greywater_core
    PRIVATE
        type_registry.cpp
        field_reflection.cpp
        serialization.cpp
        # Phase 11 Wave 11A — event bus (ADR-0023).
        events/event_bus.cpp
        # Phase 11 Wave 11B — config + CVars (ADR-0024).
        config/cvar_registry.cpp
        config/toml_binding.cpp
        config/standard_cvars.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            type_registry.hpp
            field_reflection.hpp
            serialization.hpp
            time.hpp
            # Phase 11 Wave 11A.
            events/event_bus.hpp
            events/events_core.hpp
            events/event_trace.hpp
            # Phase 11 Wave 11B.
            config/cvar.hpp
            config/cvar_registry.hpp
            config/toml_binding.hpp
            config/standard_cvars.hpp
)
