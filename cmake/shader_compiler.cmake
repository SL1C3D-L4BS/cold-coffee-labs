# Shader compilation using DXC for HLSL -> SPIR-V (Week 028)

# Find DXC executable
find_program(DXC_EXECUTABLE dxc
    DOC "DirectX Shader Compiler for HLSL -> SPIR-V"
    REQUIRED
)

if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "DXC not found. Please install DirectX Shader Compiler.")
endif()

# Function to add shader library
function(add_shader_library target_name)
    set(options "")
    set(oneValueArgs SOURCE_BASE)
    set(multiValueArgs SOURCES INCLUDE_DIRS OUTPUT_DIR)
    cmake_parse_arguments(ADD_SHADER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ADD_SHADER_SOURCES)
        message(FATAL_ERROR "add_shader_library requires SOURCES argument")
    endif()
    if(NOT ADD_SHADER_SOURCE_BASE)
        message(FATAL_ERROR "add_shader_library requires SOURCE_BASE (directory containing SOURCES paths)")
    endif()

    if(NOT ADD_SHADER_OUTPUT_DIR)
        set(ADD_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
    endif()

    # Create output directory
    file(MAKE_DIRECTORY "${ADD_SHADER_OUTPUT_DIR}")

    set(shader_outputs "")
    # Process each shader source
    foreach(shader_source ${ADD_SHADER_SOURCES})
        if(IS_ABSOLUTE "${shader_source}")
            set(shader_abs "${shader_source}")
        else()
            cmake_path(ABSOLUTE_PATH shader_source
                BASE_DIRECTORY "${ADD_SHADER_SOURCE_BASE}"
                OUTPUT_VARIABLE shader_abs)
        endif()

        get_filename_component(shader_name ${shader_abs} NAME_WE)
        get_filename_component(shader_fname ${shader_abs} NAME)

        # Compound extensions (.vert.hlsl); CMake's EXT is only the final suffix.
        if(shader_fname MATCHES "\\.vert\\.hlsl$")
            set(shader_target "vs_6_0")
            set(output_ext ".vert.spv")
        elseif(shader_fname MATCHES "\\.frag\\.hlsl$")
            set(shader_target "ps_6_0")
            set(output_ext ".frag.spv")
        elseif(shader_fname MATCHES "\\.comp\\.hlsl$")
            set(shader_target "cs_6_0")
            set(output_ext ".comp.spv")
        elseif(shader_fname MATCHES "\\.geom\\.hlsl$")
            set(shader_target "gs_6_0")
            set(output_ext ".geom.spv")
        elseif(shader_fname MATCHES "\\.tesc\\.hlsl$")
            set(shader_target "hs_6_0")
            set(output_ext ".tesc.spv")
        elseif(shader_fname MATCHES "\\.tese\\.hlsl$")
            set(shader_target "ds_6_0")
            set(output_ext ".tese.spv")
        else()
            message(WARNING "Unknown HLSL shader suffix for file: ${shader_fname}")
            continue()
        endif()

        set(output_file "${ADD_SHADER_OUTPUT_DIR}/${shader_name}${output_ext}")

        # Content hashing for incremental builds
        set(hash_file "${ADD_SHADER_OUTPUT_DIR}/${shader_name}.hash")
        set(current_hash "")

        # Calculate content hash
        file(SHA256 "${shader_abs}" current_hash)

        # Add include directories to hash calculation
        foreach(include_dir ${ADD_SHADER_INCLUDE_DIRS})
            file(GLOB_RECURSE include_files "${include_dir}/*.hlsl")
            foreach(include_file ${include_files})
                file(SHA256 ${include_file} include_hash)
                string(CONCAT current_hash "${current_hash}" "${include_hash}")
            endforeach()
        endforeach()

        # Check if we need to recompile
        set(needs_recompile TRUE)
        if(EXISTS "${hash_file}")
            file(READ "${hash_file}" stored_hash)
            if(current_hash STREQUAL stored_hash AND EXISTS "${output_file}")
                set(needs_recompile FALSE)
            endif()
        endif()

        if(needs_recompile)
            message(STATUS "Compiling shader: ${shader_abs} -> ${output_file}")

            # Build include arguments
            set(include_args "")
            foreach(include_dir ${ADD_SHADER_INCLUDE_DIRS})
                list(APPEND include_args "-I${include_dir}")
            endforeach()

            # Compile shader
            execute_process(
                COMMAND ${DXC_EXECUTABLE}
                    -spirv
                    -T ${shader_target}
                    -E main
                    -fspv-target-env=vulkan1.2
                    ${include_args}
                    "${shader_abs}"
                    -Fo "${output_file}"
                WORKING_DIRECTORY "${ADD_SHADER_SOURCE_BASE}"
                RESULT_VARIABLE compile_result
                OUTPUT_VARIABLE compile_output
                ERROR_VARIABLE compile_error
            )

            if(NOT compile_result EQUAL 0)
                message(FATAL_ERROR "Shader compilation failed for ${shader_abs}: ${compile_error}")
            endif()

            # Write hash file
            file(WRITE "${hash_file}" "${current_hash}")
        else()
            message(STATUS "Shader up to date: ${shader_abs}")
        endif()

        # Add output file to target
        list(APPEND shader_outputs "${output_file}")
    endforeach()

    # Create custom target for shader compilation
    add_custom_target(${target_name}
        DEPENDS ${shader_outputs}
        COMMENT "Compiling shaders for ${target_name}"
    )

    # Set target properties
    set_target_properties(${target_name} PROPERTIES
        FOLDER "Shaders"
    )

    # Add to main build dependencies
    add_dependencies(greywater_core ${target_name})
endfunction()

# Function to create shader hot-reload target (for development)
function(add_shader_hot_reload target_name)
    set(options WATCH)
    set(oneValueArgs WATCH_DIR OUTPUT_DIR)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ADD_SHADER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ADD_SHADER_WATCH)
        return()
    endif()

    if(NOT ADD_SHADER_WATCH_DIR)
        set(ADD_SHADER_WATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    endif()

    if(NOT ADD_SHADER_OUTPUT_DIR)
        set(ADD_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
    endif()

    # Create hot-reload script
    set(hot_reload_script "${CMAKE_BINARY_DIR}/shader_hot_reload.py")
    file(WRITE ${hot_reload_script}
"import os
import subprocess
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class ShaderHandler(FileSystemEventHandler):
    def __init__(self, output_dir, dxc_path):
        self.output_dir = output_dir
        self.dxc_path = dxc_path
        os.makedirs(output_dir, exist_ok=True)

    def on_modified(self, event):
        if event.is_directory:
            return
        
        if event.src_path.endswith('.hlsl'):
            print(f'Shader changed: {event.src_path}')
            self.compile_shader(event.src_path)

    def compile_shader(self, shader_path):
        try:
            # Determine shader type
            shader_name = os.path.splitext(os.path.basename(shader_path))[0]
            if shader_path.endswith('.vert.hlsl'):
                target = 'vs_6_0'
                ext = '.vert.spv'
            elif shader_path.endswith('.frag.hlsl'):
                target = 'ps_6_0'
                ext = '.frag.spv'
            elif shader_path.endswith('.comp.hlsl'):
                target = 'cs_6_0'
                ext = '.comp.spv'
            else:
                print(f'Unknown shader type: {shader_path}')
                return

            output_path = os.path.join(self.output_dir, shader_name + ext)
            
            # Compile shader
            result = subprocess.run([
                self.dxc_path,
                '-spirv',
                '-T', target,
                '-E', 'main',
                '-fspv-target-env=vulkan1.2',
                shader_path,
                '-Fo', output_path
            ], capture_output=True, text=True)

            if result.returncode != 0:
                print(f'Compilation failed: {result.stderr}')
            else:
                print(f'Compiled: {output_path}')

        except Exception as e:
            print(f'Error compiling shader: {e}')

if __name__ == '__main__':
    watch_dir = '${ADD_SHADER_WATCH_DIR}'
    output_dir = '${ADD_SHADER_OUTPUT_DIR}'
    dxc_path = '${DXC_EXECUTABLE}'
    
    handler = ShaderHandler(output_dir, dxc_path)
    observer = Observer()
    observer.schedule(handler, watch_dir, recursive=True)
    observer.start()
    
    print(f'Watching {watch_dir} for shader changes...')
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
"
    )

    # Create hot-reload target
    add_custom_target(shader_hot_reload_${target_name}
        COMMAND python ${hot_reload_script}
        COMMENT "Starting shader hot-reload for ${target_name}"
    )

    set_target_properties(shader_hot_reload_${target_name} PROPERTIES
        FOLDER "Shaders"
    )
endfunction()
