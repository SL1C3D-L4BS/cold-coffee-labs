# sandbox/ — engine feature test bed.
# Phase 1 week 004 deliverable: hello-triangle (First Light Milestone 1A).
# docs/10 §3 Week 02 / §6 First-week code generation.

add_executable(gw_sandbox)
target_sources(gw_sandbox PRIVATE main.cpp)

target_link_libraries(gw_sandbox
    PRIVATE
        greywater::core
        glfw
        volk::volk
        GPUOpen::VulkanMemoryAllocator
        Vulkan::Headers
)

# Link against the platform loader: libvulkan (Linux) / vulkan-1.lib (Win)
# so validation layers can attach at runtime.
target_link_libraries(gw_sandbox PRIVATE Vulkan::Vulkan)

target_compile_definitions(gw_sandbox PRIVATE
    VK_NO_PROTOTYPES
    GLFW_INCLUDE_VULKAN=0
    $<$<CONFIG:Debug>:GW_VK_VALIDATION=1>
)

# --- Shader compilation -------------------------------------------------------
# docs/10 §6.6 + docs/00 §3 (Shaders): HLSL primary via DXC, GLSL supported.
# Phase 1 uses the Vulkan SDK's glslangValidator for bootstrapping — DXC
# integration lands in Phase 5 (Shader toolchain week 028).
find_program(GW_GLSLANG glslangValidator HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)
if(NOT GW_GLSLANG)
    message(FATAL_ERROR
        "glslangValidator not found. Set VULKAN_SDK or install the Vulkan SDK "
        "(docs/11 §Hot commands, docs/00 §2.2).")
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
