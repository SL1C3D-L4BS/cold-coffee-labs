# sandbox/ — engine feature test bed.
# Phase 1 week 004 deliverable: hello-triangle (First Light Milestone 1A).
# docs/01_CONSTITUTION_AND_PROGRAM.md (merged pre-dev plan) §3 Week 02 / §6.

add_executable(gw_sandbox)
target_sources(gw_sandbox PRIVATE main.cpp)

target_link_libraries(gw_sandbox
    PRIVATE
        greywater::core
        glfw
        volk::volk              # volk dynamically loads vulkan-1.dll via LoadLibrary
        GPUOpen::VulkanMemoryAllocator
        Vulkan::Headers         # header-only; no import stubs
)
# Do NOT link Vulkan::Vulkan (vulkan-1.lib) — volk owns the loader path.
# Linking both causes the static-lib resolver to pick volk's null PFN_* variables
# over vulkan-1.lib's IAT stubs, crashing at address 0x0 before main().

target_compile_definitions(gw_sandbox PRIVATE
    VK_NO_PROTOTYPES
    GLFW_INCLUDE_VULKAN=0
    $<$<CONFIG:Debug>:GW_VK_VALIDATION=1>
)

# --- Shader compilation -------------------------------------------------------
# docs/10_APPENDIX_ADRS_AND_REFERENCES.md §6.6 + docs/01_CONSTITUTION_AND_PROGRAM.md §3 (Shaders): HLSL primary via DXC, GLSL supported.
# Phase 1 uses the Vulkan SDK's glslangValidator for bootstrapping — DXC
# integration lands in Phase 5 (Shader toolchain week 028).
find_program(GW_GLSLANG glslangValidator HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)
if(NOT GW_GLSLANG)
    message(FATAL_ERROR
        "glslangValidator not found. Set VULKAN_SDK or install the Vulkan SDK "
        "(docs/01_CONSTITUTION_AND_PROGRAM.md § handoff / §2.2).")
endif()

set(GW_SHADER_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${GW_SHADER_OUT_DIR}")

function(gw_compile_shader TARGET SHADER_PATH)
    get_filename_component(SHADER_NAME "${SHADER_PATH}" NAME)
    set(SPV "${GW_SHADER_OUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT  "${SPV}"
        COMMAND "${GW_GLSLANG}" --target-env vulkan1.2 -o "${SPV}"
                "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_PATH}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_PATH}"
        COMMENT "Compiling ${SHADER_NAME} -> SPIR-V"
        VERBATIM
    )
    target_sources(${TARGET} PRIVATE "${SPV}")
    set_source_files_properties("${SPV}" PROPERTIES GENERATED TRUE)
endfunction()

gw_compile_shader(gw_sandbox "shaders/triangle.vert")
gw_compile_shader(gw_sandbox "shaders/triangle.frag")

target_compile_definitions(gw_sandbox PRIVATE
    GW_SHADER_DIR="${GW_SHADER_OUT_DIR}"
)

set_target_properties(gw_sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
