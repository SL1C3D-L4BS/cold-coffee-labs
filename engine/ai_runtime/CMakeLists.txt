# engine/ai_runtime/ — Phase 26 scaffold (ADR-0095, ADR-0096, ADR-0097, ADR-0098).
#
# Bounded on-device C++ inference under the docs/01 §2.2.2 carve-out.
# See docs/05 §14 (deterministic ML coding standard) and docs/06 §3.32.
#
# Monolithic pattern: sources are appended to `greywater_core` directly via
# FILE_SET HEADERS (matches audit-2026 finding #18). A standalone test harness
# target is provided for doctest; no public alias lib is needed.

target_sources(greywater_core
    PRIVATE
        inference_runtime.cpp
        model_registry.cpp
        director_policy.cpp
        music_symbolic.cpp
        material_eval.cpp
        ai_cvars.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            ai_runtime_types.hpp
            inference_runtime.hpp
            model_registry.hpp
            director_policy.hpp
            music_symbolic.hpp
            material_eval.hpp
            ai_cvars.hpp
)
