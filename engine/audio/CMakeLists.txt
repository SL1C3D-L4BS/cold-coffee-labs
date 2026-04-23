# engine/audio/ — Phase 10 Runtime I: audio.
# Owned ADRs: 0017 (service), 0018 (spatial), 0019 (streaming), 0022 (cross).
#
# Optional dependencies gated behind GW_AUDIO_* flags default OFF; the
# null + stub backends compile everywhere.

add_library(greywater_audio STATIC)
add_library(greywater::audio ALIAS greywater_audio)

target_compile_features(greywater_audio PUBLIC cxx_std_23)

target_include_directories(greywater_audio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_audio
    PUBLIC
        greywater::core
)

target_sources(greywater_audio
    PRIVATE
        voice_pool.cpp
        mixer_graph.cpp
        spatial.cpp
        propagation.cpp
        kaudio_format.cpp
        decoder_registry.cpp
        music_streamer.cpp
        audio_service.cpp
        audio_backend_null.cpp
        audio_mixer_system.cpp
        # Phase 21 W139 — Martyrdom audio stems + dialogue trigger events (ADR-0108).
        martyrdom_audio_controller.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            audio_types.hpp
            audio_backend.hpp
            voice_pool.hpp
            mixer_graph.hpp
            atmosphere_filter.hpp
            spatial.hpp
            propagation.hpp
            kaudio_format.hpp
            decoder_registry.hpp
            music_streamer.hpp
            audio_service.hpp
            audio_mixer_system.hpp
            voice_capture.hpp
            voice_playback.hpp
            martyrdom_audio_controller.hpp
)

# martyrdom_audio_controller consumes narrative types; the narrative headers
# are appended to greywater_core, so greywater::core already provides them.

# Optional: miniaudio production backend
if(DEFINED GW_AUDIO_MINIAUDIO AND GW_AUDIO_MINIAUDIO)
    target_compile_definitions(greywater_audio PUBLIC GW_AUDIO_MINIAUDIO=1)
endif()

if(DEFINED GW_AUDIO_STEAM AND GW_AUDIO_STEAM)
    target_compile_definitions(greywater_audio PUBLIC GW_AUDIO_STEAM=1)
endif()
