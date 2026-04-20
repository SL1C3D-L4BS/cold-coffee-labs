#include "shader_manager.hpp"
#include <expected>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>

namespace gw::render::shader {

using gw::render::frame_graph::FrameGraphError;
using gw::render::frame_graph::FrameGraphErrorType;

ShaderManager::ShaderManager(VkDevice device)
    : device_(device) {
    
    // Set up cache directory
    cache_dir_ = std::filesystem::temp_directory_path() / "greywater_shader_cache";
    std::filesystem::create_directories(cache_dir_);
}

ShaderManager::~ShaderManager() {
    // Clean up all shader modules
    for (auto& [name, shader_info] : shaders_) {
        if (shader_info->module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, shader_info->module, nullptr);
        }
    }
}

Result<VkShaderModule> ShaderManager::load_shader(
    const std::string& name,
    const std::filesystem::path& source_path,
    VkShaderStageFlagBits stage) {
    
    // Check if already loaded
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        return it->second->module;
    }
    
    // Create shader info
    auto shader_info = std::make_unique<ShaderInfo>(name, stage);
    shader_info->source_path = source_path;
    shader_info->compiled_path = get_cache_path(name, stage);
    
    // Compile shader
    auto compile_result = compile_shader(source_path, stage);
    if (!compile_result) {
        return std::unexpected(compile_result.error());
    }
    
    auto& compilation = compile_result.value();
    if (!compilation.success) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Shader compilation failed: " + compilation.error_message
        });
    }
    
    // Create Vulkan shader module
    auto module_result = create_shader_module(*shader_info, compilation.spirv_code);
    if (!module_result) {
        return std::unexpected(module_result.error());
    }
    
    shader_info->content_hash = compilation.content_hash;
    shader_info->is_loaded = true;
    
    // Save to cache
    auto cache_result = save_to_cache(name, stage, compilation.spirv_code, compilation.content_hash);
    if (!cache_result) {
        // Non-fatal, continue
    }
    
    VkShaderModule module = shader_info->module;
    shaders_[name] = std::move(shader_info);
    
    return module;
}

Result<VkShaderModule> ShaderManager::get_shader(const std::string& name) {
    auto it = shaders_.find(name);
    if (it == shaders_.end()) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidResourceHandle,
            "Shader not found: " + name
        });
    }
    
    return it->second->module;
}

Result<std::monostate> ShaderManager::enable_hot_reload(const std::filesystem::path& watch_dir) {
    if (!std::filesystem::exists(watch_dir)) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Watch directory does not exist: " + watch_dir.string()
        });
    }
    
    hot_reload_enabled_ = true;
    watch_directory_ = watch_dir;
    
    // Initial scan
    return scan_directory();
}

Result<std::monostate> ShaderManager::disable_hot_reload() {
    hot_reload_enabled_ = false;
    last_modified_times_.clear();
    return std::monostate{};
}

Result<std::monostate> ShaderManager::check_for_changes() {
    if (!hot_reload_enabled_) {
        return std::monostate{};
    }
    
    return scan_directory();
}

Result<std::monostate> ShaderManager::clear_cache() {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cache_dir_)) {
            std::filesystem::remove_all(entry);
        }
        return std::monostate{};
    } catch (const std::exception& e) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to clear cache: " + std::string(e.what())
        });
    }
}

Result<std::monostate> ShaderManager::preload_cache(const std::filesystem::path& cache_dir) {
    if (!std::filesystem::exists(cache_dir)) {
        return std::monostate{};  // Cache doesn't exist yet
    }
    
    // Copy cache files to our cache directory
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cache_dir)) {
            if (entry.path().extension() == ".spv") {
                std::filesystem::copy_file(
                    entry.path(),
                    cache_dir_ / entry.path().filename(),
                    std::filesystem::copy_options::overwrite_existing
                );
            }
        }
        return std::monostate{};
    } catch (const std::exception& e) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to preload cache: " + std::string(e.what())
        });
    }
}

VkShaderStageFlagBits ShaderManager::get_stage_from_extension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    
    if (ext == ".vert.hlsl" || ext == ".vert") {
        return VK_SHADER_STAGE_VERTEX_BIT;
    } else if (ext == ".frag.hlsl" || ext == ".frag") {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    } else if (ext == ".comp.hlsl" || ext == ".comp") {
        return VK_SHADER_STAGE_COMPUTE_BIT;
    } else if (ext == ".geom.hlsl" || ext == ".geom") {
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    } else if (ext == ".tesc.hlsl" || ext == ".tesc") {
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    } else if (ext == ".tese.hlsl" || ext == ".tese") {
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }
    
    return VK_SHADER_STAGE_ALL;  // Unknown
}

std::string ShaderManager::calculate_content_hash(const std::string& content) {
    constexpr uint64_t fnv_offset = 14695981039346656037ull;
    constexpr uint64_t fnv_prime = 1099511628211ull;
    uint64_t h = fnv_offset;
    for (unsigned char byte : content) {
        h ^= byte;
        h *= fnv_prime;
    }
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << h;
    return ss.str();
}

Result<CompilationResult> ShaderManager::compile_shader(
    const std::filesystem::path& source_path,
    VkShaderStageFlagBits stage) {
    
    CompilationResult result{};
    
    // Read source file
    std::ifstream file(source_path, std::ios::in | std::ios::binary);
    if (!file) {
        result.success = false;
        result.error_message = "Failed to read shader source: " + source_path.string();
        return result;
    }
    
    std::string source_content(std::istreambuf_iterator<char>(file), {});
    file.close();
    
    // Calculate content hash
    result.content_hash = calculate_content_hash(source_content);
    
    // Check cache first
    auto cached_result = load_from_cache(
        source_path.stem().string(), stage, result.content_hash);
    if (cached_result) {
        result.success = true;
        result.spirv_code = cached_result.value();
        return result;
    }
    
    // Compile with DXC
    auto dxc_result = compile_with_dxc(source_path, stage);
    if (!dxc_result) {
        result.success = false;
        result.error_message = "DXC compilation failed";
        return result;
    }
    
    result.success = true;
    result.spirv_code = dxc_result.value();
    return result;
}

Result<std::monostate> ShaderManager::create_shader_module(
    ShaderInfo& shader_info,
    const std::vector<uint32_t>& spirv_code) {
    
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = spirv_code.size() * sizeof(uint32_t);
    create_info.pCode = spirv_code.data();
    
    VkResult result = vkCreateShaderModule(device_, &create_info, nullptr, &shader_info.module);
    if (result != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to create shader module"
        });
    }
    
    return std::monostate{};
}

std::filesystem::path ShaderManager::get_cache_path(const std::string& name, VkShaderStageFlagBits stage) {
    std::string stage_suffix;
    
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT: stage_suffix = ".vert.spv"; break;
        case VK_SHADER_STAGE_FRAGMENT_BIT: stage_suffix = ".frag.spv"; break;
        case VK_SHADER_STAGE_COMPUTE_BIT: stage_suffix = ".comp.spv"; break;
        case VK_SHADER_STAGE_GEOMETRY_BIT: stage_suffix = ".geom.spv"; break;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: stage_suffix = ".tesc.spv"; break;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: stage_suffix = ".tese.spv"; break;
        default: stage_suffix = ".spv"; break;
    }
    
    return cache_dir_ / (name + stage_suffix);
}

Result<std::monostate> ShaderManager::save_to_cache(
    const std::string& name,
    VkShaderStageFlagBits stage,
    const std::vector<uint32_t>& spirv_code,
    const std::string& content_hash) {
    
    try {
        std::filesystem::path cache_path = get_cache_path(name, stage);
        std::ofstream file(cache_path, std::ios::out | std::ios::binary);
        
        if (!file) {
            return std::unexpected(FrameGraphError{
                FrameGraphErrorType::CompilationFailed,
                "Failed to write to cache"
            });
        }
        
        file.write(reinterpret_cast<const char*>(spirv_code.data()), 
                 spirv_code.size() * sizeof(uint32_t));
        file.close();
        
        // Write hash file
        std::filesystem::path hash_path = cache_path.string() + ".hash";
        std::ofstream hash_file(hash_path);
        hash_file << content_hash;
        
        return std::monostate{};
    } catch (const std::exception& e) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to save to cache: " + std::string(e.what())
        });
    }
}

Result<std::vector<uint32_t>> ShaderManager::load_from_cache(
    const std::string& name,
    VkShaderStageFlagBits stage,
    const std::string& expected_hash) {
    
    std::filesystem::path cache_path = get_cache_path(name, stage);
    std::filesystem::path hash_path = cache_path.string() + ".hash";
    
    if (!std::filesystem::exists(cache_path) || !std::filesystem::exists(hash_path)) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Cache entry not found"
        });
    }
    
    // Check hash
    std::ifstream hash_file(hash_path);
    std::string cached_hash;
    std::getline(hash_file, cached_hash);
    hash_file.close();
    
    if (cached_hash != expected_hash) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Cache entry hash mismatch"
        });
    }
    
    // Load SPIR-V code
    std::ifstream file(cache_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to read cached shader"
        });
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint32_t> spirv_code(size / sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(spirv_code.data()), size);
    
    return spirv_code;
}

Result<std::monostate> ShaderManager::scan_directory() {
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(watch_directory_)) {
            if (entry.is_regular_file() && 
                (entry.path().extension() == ".hlsl" || 
                 entry.path().extension() == ".vert" ||
                 entry.path().extension() == ".frag" ||
                 entry.path().extension() == ".comp")) {
                
                if (has_file_changed(entry.path())) {
                    std::string shader_name = entry.path().stem().string();
                    auto reload_result = reload_shader(shader_name);
                    if (!reload_result) {
                        // Log error but continue
                        continue;
                    }
                }
            }
        }
        return std::monostate{};
    } catch (const std::exception& e) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to scan directory: " + std::string(e.what())
        });
    }
}

bool ShaderManager::has_file_changed(const std::filesystem::path& file_path) {
    auto last_write = std::filesystem::last_write_time(file_path);
    std::string path_str = file_path.string();
    
    auto it = std::find_if(last_modified_times_.begin(), last_modified_times_.end(),
        [&path_str](const auto& pair) { return pair.first == path_str; });
    
    if (it == last_modified_times_.end()) {
        last_modified_times_.emplace_back(path_str, last_write);
        return true;  // New file
    }
    
    if (it->second != last_write) {
        it->second = last_write;
        return true;
    }
    
    return false;
}

Result<std::monostate> ShaderManager::reload_shader(const std::string& name) {
    auto it = shaders_.find(name);
    if (it == shaders_.end()) {
        return std::monostate{};  // Not loaded yet
    }
    
    auto& shader_info = it->second;
    
    // Recompile
    auto compile_result = compile_shader(shader_info->source_path, shader_info->stage);
    if (!compile_result) {
        return std::unexpected(compile_result.error());
    }
    
    auto& compilation = compile_result.value();
    if (!compilation.success) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Shader hot-reload compilation failed: " + compilation.error_message
        });
    }
    
    // Destroy old module
    if (shader_info->module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, shader_info->module, nullptr);
    }
    
    // Create new module
    auto module_result = create_shader_module(*shader_info, compilation.spirv_code);
    if (!module_result) {
        return std::unexpected(module_result.error());
    }
    
    // Update cache
    (void)save_to_cache(name, shader_info->stage, compilation.spirv_code, compilation.content_hash);
    
    return std::monostate{};
}

Result<std::vector<uint32_t>> ShaderManager::compile_with_dxc(
    const std::filesystem::path& source_path,
    VkShaderStageFlagBits stage) {
    
    std::string target = get_dxc_target(stage);
    std::string entry_point = get_dxc_entry_point(source_path);
    
    // Build DXC command
    std::string command = "dxc -spirv -T " + target + " -E " + entry_point +
                         " -fspv-target-env=vulkan1.2 " + source_path.string() +
                         " -Fo " + source_path.string() + ".spv";
    
    // Execute DXC
    int result = std::system(command.c_str());
    if (result != 0) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "DXC compilation failed with code: " + std::to_string(result)
        });
    }
    
    // Load compiled SPIR-V
    std::filesystem::path spv_path = source_path.string() + ".spv";
    std::ifstream file(spv_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Failed to read compiled SPIR-V"
        });
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint32_t> spirv_code(size / sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(spirv_code.data()), size);
    
    // Clean up temporary file
    std::filesystem::remove(spv_path);
    
    return spirv_code;
}

std::string ShaderManager::get_dxc_target(VkShaderStageFlagBits stage) {
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT: return "vs_6_0";
        case VK_SHADER_STAGE_FRAGMENT_BIT: return "ps_6_0";
        case VK_SHADER_STAGE_COMPUTE_BIT: return "cs_6_0";
        case VK_SHADER_STAGE_GEOMETRY_BIT: return "gs_6_0";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "hs_6_0";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "ds_6_0";
        default: return "vs_6_0";
    }
}

std::string ShaderManager::get_dxc_entry_point(const std::filesystem::path& source_path) {
    // For now, assume "main" - could be extended to parse from source
    return "main";
}

} // namespace gw::render::shader
