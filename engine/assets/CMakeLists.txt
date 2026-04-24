# engine/assets/CMakeLists.txt — Phase 6: Asset Pipeline & Runtime DB
# Phase 6 spec §11.1.

add_library(gw_assets STATIC)

target_sources(gw_assets
    PRIVATE
        cook_trust.cpp
        image_decode_rgba8.cpp
        asset_db.cpp
        asset_loader.cpp
        mesh_asset.cpp
        texture_asset.cpp
        vfs/virtual_fs.cpp
        vfs/native_mount.cpp
        vfs/watcher.cpp
        cook/cook_manifest.cpp
        cook/cook_registry.cpp
        cook/mesh_cooker.cpp
        cook/texture_cooker.cpp
)

# Public include: lets dependents use #include "engine/assets/..."
target_include_directories(gw_assets
    PUBLIC
        "${CMAKE_SOURCE_DIR}"               # project root
)

# Private includes: third-party headers not part of the public ABI.
target_include_directories(gw_assets
    PRIVATE
        "${orlp_ed25519_SOURCE_DIR}/src"
        "${CMAKE_BINARY_DIR}/_deps/volk-src"
        "${CMAKE_BINARY_DIR}/_deps/VulkanMemoryAllocator-src/include"
        "${CMAKE_BINARY_DIR}/_deps/basisu-src"
        "${CMAKE_BINARY_DIR}/_deps/basisu-src/transcoder"
        "${CMAKE_BINARY_DIR}/_deps/basisu-src/encoder"
)

target_link_libraries(gw_assets
    PUBLIC
        greywater::core
    PRIVATE
        gw_orlp_ed25519
        volk
        VulkanMemoryAllocator
        fastgltf
        meshoptimizer
        mikktspace
        basisu_encoder
        xxhash
        stb
)

# Alias so tools and tests use greywater::assets.
add_library(greywater::assets ALIAS gw_assets)
