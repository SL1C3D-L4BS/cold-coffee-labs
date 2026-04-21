# engine/net/ — Phase 14 (ADR-0047..0055).
#
# Deterministic networking + multiplayer: PIMPL facade over a null
# loopback transport (always available) and optional GameNetworkingSockets
# transport (GW_ENABLE_GNS). Voice pipeline routes through Opus when
# GW_ENABLE_OPUS is ON; otherwise PCM passthrough is used. No third-party
# network, codec, or matchmaking headers are reachable from callers of
# these headers.

add_library(greywater_net STATIC
    transport_null.cpp
    link_sim.cpp
    snapshot.cpp
    replication.cpp
    reliability.cpp
    desync_detector.cpp
    lockstep.cpp
    voice.cpp
    session.cpp
    network_world.cpp
    net_cvars.cpp
    console_commands.cpp
)
add_library(greywater::net ALIAS greywater_net)

target_compile_features(greywater_net PUBLIC cxx_std_23)

target_include_directories(greywater_net
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(greywater_net
    PUBLIC
        greywater::core
        greywater::console
        glm::glm
)

# Optional GameNetworkingSockets backend.
if(GW_ENABLE_GNS AND TARGET GameNetworkingSockets)
    target_compile_definitions(greywater_net PUBLIC GW_NET_GNS=1)
    target_link_libraries(greywater_net PRIVATE GameNetworkingSockets)
    target_sources(greywater_net PRIVATE impl/gns_bridge.cpp)
endif()

# Optional Opus voice codec.
if(GW_ENABLE_OPUS AND TARGET opus)
    target_compile_definitions(greywater_net PUBLIC GW_ENABLE_OPUS=1)
    target_link_libraries(greywater_net PRIVATE opus)
endif()

target_sources(greywater_net
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            net_types.hpp
            net_config.hpp
            events_net.hpp
            transport.hpp
            snapshot.hpp
            replication.hpp
            reliability.hpp
            lag_comp.hpp
            desync_detector.hpp
            lockstep.hpp
            voice.hpp
            session.hpp
            network_world.hpp
            net_cvars.hpp
            console_commands.hpp
)
