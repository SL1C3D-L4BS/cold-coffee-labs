# Vendored tiny_sha3 — FIPS-202 Keccak/SHA-3/SHAKE reference implementation.
# Upstream: https://github.com/mjosaarinen/tiny_sha3
# Licence:  MIT (see LICENSE in this directory).
#
# Greywater: used by engine/core/crypto/sha3.cpp as the C-ABI backend for
# `gw::crypto::sha3_256_digest`. Vendored verbatim; any upstream merge goes
# through an ADR per docs/10 §Third-party dependency policy.

add_library(tiny_sha3 STATIC
    tiny_sha3.c
)

# Expose tiny_sha3.h via its vendored directory so consumers include it as
# #include "tiny_sha3.h" — there is exactly one call-site, engine/core/crypto/sha3.cpp.
target_include_directories(tiny_sha3 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Third-party C: do not promote our warning-as-error policy onto a vendored
# reference implementation we do not control.
if(MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    target_compile_options(tiny_sha3 PRIVATE /w)
else()
    target_compile_options(tiny_sha3 PRIVATE -w)
endif()

add_library(greywater::tiny_sha3 ALIAS tiny_sha3)
