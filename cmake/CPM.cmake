# CPM.cmake bootstrap — CPM is the package manager Greywater uses for
# pinned, SHA-anchored third-party C++ dependencies. See docs/10 §6.3.
#
# This file is a tiny shim that downloads the real CPM.cmake on first
# configure and caches it; subsequent configures are offline. No writes
# to the user's global CMake module path.
#
# CPM source / license: https://github.com/cpm-cmake/CPM.cmake (MIT).

set(CPM_DOWNLOAD_VERSION 0.40.8)
set(CPM_HASH_SUM "78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791")

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

get_filename_component(CPM_DOWNLOAD_LOCATION "${CPM_DOWNLOAD_LOCATION}" ABSOLUTE)

function(gw_download_cpm)
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION} to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
        EXPECTED_HASH "SHA256=${CPM_HASH_SUM}"
        TLS_VERIFY ON)
endfunction()

if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    gw_download_cpm()
else()
    file(READ "${CPM_DOWNLOAD_LOCATION}" _existing LIMIT 1024)
    if(NOT _existing)
        gw_download_cpm()
    endif()
endif()

include("${CPM_DOWNLOAD_LOCATION}")
