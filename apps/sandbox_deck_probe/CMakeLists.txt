# apps/sandbox_deck_probe/CMakeLists.txt — Part C §22 scaffold (ADR-0113).
#
# Nightly CI runs this headless under `steamrt-sniper` to verify the
# Steam Deck Verified checklist: no launcher on first run, no M+KB glyph
# leaks, all functions reachable via default controller bindings, no
# device-compatibility warnings.

add_executable(sandbox_deck_probe main.cpp)

target_link_libraries(sandbox_deck_probe
    PRIVATE
        greywater::core
)

target_include_directories(sandbox_deck_probe PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(sandbox_deck_probe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
