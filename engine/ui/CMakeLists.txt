# engine/ui/ — Phase 11 Waves 11D + 11E: UI services.
# Owned ADRs: 0026 (UIService), 0027 (Vulkan HAL backend + settings binder),
# 0028 (text shaping pipeline).
#
# Heavy deps (RmlUi / FreeType / HarfBuzz) are gated behind CMake options
# that default OFF so the generic CI build tree stays lean. The `gw_runtime`
# preset turns them on; see cmake/dependencies.cmake.

add_library(greywater_ui STATIC)
add_library(greywater::ui ALIAS greywater_ui)

target_compile_features(greywater_ui PUBLIC cxx_std_23)

target_include_directories(greywater_ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_ui
    PUBLIC
        greywater::core
)

target_sources(greywater_ui
    PRIVATE
        locale_bridge.cpp
        font_library.cpp
        glyph_atlas.cpp
        text_shaper.cpp
        settings_binder.cpp
        ui_service.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            ui_types.hpp
            locale_bridge.hpp
            font_library.hpp
            glyph_atlas.hpp
            text_shaper.hpp
            settings_binder.hpp
            ui_service.hpp
)

# --- Optional: RmlUi backend ---------------------------------------------
# Real RmlUi integration lives in engine/ui/backends/rml_render_hal once
# the dep is pulled (see ADR-0027). Phase-11 CI ships the null backend;
# the gated build flips GW_UI_RMLUI=1 and pulls the CPM-fetched RmlUi.
if(DEFINED GW_UI_RMLUI AND GW_UI_RMLUI)
    target_compile_definitions(greywater_ui PUBLIC GW_UI_RMLUI=1)
    # TODO(phase-11-11E): add RmlUi link + backend .cpp files once the
    # CPM fetch is wired in cmake/dependencies.cmake.
endif()

if(DEFINED GW_UI_FREETYPE AND GW_UI_FREETYPE)
    target_compile_definitions(greywater_ui PUBLIC GW_UI_FREETYPE=1)
endif()

if(DEFINED GW_UI_HARFBUZZ AND GW_UI_HARFBUZZ)
    target_compile_definitions(greywater_ui PUBLIC GW_UI_HARFBUZZ=1)
endif()
