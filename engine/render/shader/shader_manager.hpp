#pragma once

#include <volk.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <functional>

#include "../frame_graph/error.hpp"

namespace gw::render::shader {

template<typename T>
using Result = gw::render::frame_graph::Result<T>;

// Shader information
struct ShaderInfo {
    std::string name;
    std::filesystem::path source_path;
    std::filesystem::path compiled_path;
    std::string content_hash;
    VkShaderStageFlagBits stage;
    VkShaderModule module = VK_NULL_HANDLE;
    bool is_loaded = false;
    
    ShaderInfo(const std::string& shader_name, VkShaderStageFlagBits shader_stage)
        : name(shader_name), stage(shader_stage) {}
};

// Shader compilation result
struct CompilationResult {
    bool success = false;
    std::string error_message;
    std::vector<uint32_t> spirv_code;
    std::string content_hash;
};

// Shader manager with content-hashed cache and hot-reload (Week 028)
class ShaderManager {
public:
    explicit ShaderManager(VkDevice device);
    ~ShaderManager();
    
    // Non-copyable, movable
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = default;
    ShaderManager& operator=(ShaderManager&&) = default;
    
    // Shader loading and compilation
    Result<VkShaderModule> load_shader(
        const std::string& name,
        const std::filesystem::path& source_path,
        VkShaderStageFlagBits stage);
    
    Result<VkShaderModule> get_shader(const std::string& name);
    
    // Hot-reload functionality
    Result<std::monostate> enable_hot_reload(const std::filesystem::path& watch_dir);
    Result<std::monostate> disable_hot_reload();
    Result<std::monostate> check_for_changes();
    
    // Cache management
    Result<std::monostate> clear_cache();
    Result<std::monostate> preload_cache(const std::filesystem::path& cache_dir);
    
    // Utility functions
    static VkShaderStageFlagBits get_stage_from_extension(const std::filesystem::path& path);
    static std::string calculate_content_hash(const std::string& content);
    
private:
    VkDevice device_;
    
    // Shader storage
    std::unordered_map<std::string, std::unique_ptr<ShaderInfo>> shaders_;
    
    // Cache directory
    std::filesystem::path cache_dir_;
    
    // Hot-reload state
    bool hot_reload_enabled_ = false;
    std::filesystem::path watch_directory_;
    std::vector<std::pair<std::string, std::filesystem::file_time_type>> last_modified_times_;
    
    // Compilation
    Result<CompilationResult> compile_shader(
        const std::filesystem::path& source_path,
        VkShaderStageFlagBits stage);
    
    Result<std::monostate> create_shader_module(
        ShaderInfo& shader_info,
        const std::vector<uint32_t>& spirv_code);
    
    // Cache helpers
    std::filesystem::path get_cache_path(const std::string& name, VkShaderStageFlagBits stage);
    Result<std::monostate> save_to_cache(
        const std::string& name,
        VkShaderStageFlagBits stage,
        const std::vector<uint32_t>& spirv_code,
        const std::string& content_hash);
    
    Result<std::vector<uint32_t>> load_from_cache(
        const std::string& name,
        VkShaderStageFlagBits stage,
        const std::string& expected_hash);
    
    // Hot-reload helpers
    Result<std::monostate> scan_directory();
    bool has_file_changed(const std::filesystem::path& file_path);
    Result<std::monostate> reload_shader(const std::string& name);
    
    // DXC integration
    Result<std::vector<uint32_t>> compile_with_dxc(
        const std::filesystem::path& source_path,
        VkShaderStageFlagBits stage);
    
    std::string get_dxc_target(VkShaderStageFlagBits stage);
    std::string get_dxc_entry_point(const std::filesystem::path& source_path);
};

// Hot-reload callback type
using ShaderReloadCallback = std::function<void(const std::string&, VkShaderModule)>;

} // namespace gw::render::shader
