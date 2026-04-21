#include "shader_manager.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>

// Content hash — SHA-256 via a simple FNV-1a 64-bit fold for Phase 5.
// Production upgrade: swap for std::hash or a proper SHA-256 in Week 032+.
static std::string fnv1a_hex(const uint8_t* data, size_t size) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) {
        h ^= static_cast<uint64_t>(data[i]);
        h *= 1099511628211ull;
    }
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << h;
    return ss.str();
}

namespace gw::render::shader {

using gw::render::frame_graph::FrameGraphError;
using gw::render::frame_graph::FrameGraphErrorType;

ShaderManager::ShaderManager(VkDevice device) : device_(device) {}

ShaderManager::~ShaderManager() {
    for (auto& [name, info] : shaders_) {
        if (info && info->module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, info->module, nullptr);
            info->module = VK_NULL_HANDLE;
        }
    }
}

// ---------------------------------------------------------------------------
// load_shader — read pre-compiled .spv from disk, create VkShaderModule.
// The source_path is expected to be the .spv output of the build-time DXC run.
// ---------------------------------------------------------------------------
Result<VkShaderModule> ShaderManager::load_shader(
        const std::string& name,
        const std::filesystem::path& source_path,
        VkShaderStageFlagBits stage) {
    // Check cache first
    auto cached = shaders_.find(name);
    if (cached != shaders_.end() && cached->second->is_loaded) {
        return cached->second->module;
    }

    // Load SPIR-V bytes from disk
    std::ifstream file(source_path, std::ios::ate | std::ios::binary);
    if (!file) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Cannot open shader SPIR-V: " + source_path.string()
        });
    }
    const size_t file_size = static_cast<size_t>(file.tellg());
    if (file_size == 0 || file_size % 4 != 0) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Invalid SPIR-V file (bad size): " + source_path.string()
        });
    }

    std::vector<uint32_t> spirv(file_size / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(spirv.data()), static_cast<std::streamsize>(file_size));
    file.close();

    const std::string hash = fnv1a_hex(
        reinterpret_cast<const uint8_t*>(spirv.data()), file_size);

    // Create ShaderInfo entry
    auto info = std::make_unique<ShaderInfo>(name, stage);
    info->source_path   = source_path;
    info->content_hash  = hash;

    auto r = create_shader_module(*info, spirv);
    if (!r) return std::unexpected(r.error());

    VkShaderModule mod = info->module;
    shaders_[name] = std::move(info);

    std::fprintf(stdout, "[shader] loaded '%s'  hash=%s\n", name.c_str(), hash.c_str());
    return mod;
}

Result<VkShaderModule> ShaderManager::get_shader(const std::string& name) {
    auto it = shaders_.find(name);
    if (it == shaders_.end() || !it->second->is_loaded) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidResourceHandle,
            "Shader not loaded: " + name
        });
    }
    return it->second->module;
}

Result<std::monostate> ShaderManager::enable_hot_reload(
        const std::filesystem::path& watch_dir) {
    watch_directory_    = watch_dir;
    hot_reload_enabled_ = true;
    last_modified_times_.clear();

    // Record current mtimes for all loaded shaders
    for (const auto& [name, info] : shaders_) {
        if (!info->source_path.empty() &&
            std::filesystem::exists(info->source_path)) {
            last_modified_times_.emplace_back(
                name, std::filesystem::last_write_time(info->source_path));
        }
    }
    return std::monostate{};
}

Result<std::monostate> ShaderManager::disable_hot_reload() {
    hot_reload_enabled_ = false;
    last_modified_times_.clear();
    return std::monostate{};
}

Result<std::monostate> ShaderManager::check_for_changes() {
    if (!hot_reload_enabled_) return std::monostate{};

    for (auto& [name, mtime] : last_modified_times_) {
        auto it = shaders_.find(name);
        if (it == shaders_.end()) continue;

        const auto& path = it->second->source_path;
        if (!std::filesystem::exists(path)) continue;

        const auto current_mtime = std::filesystem::last_write_time(path);
        if (current_mtime != mtime) {
            mtime = current_mtime;
            auto r = reload_shader(name);
            if (!r) {
                std::fprintf(stderr, "[shader] hot-reload failed for '%s': %s\n",
                             name.c_str(), r.error().message.c_str());
            } else {
                std::fprintf(stdout, "[shader] hot-reloaded '%s'\n", name.c_str());
            }
        }
    }
    return std::monostate{};
}

Result<std::monostate> ShaderManager::clear_cache() {
    for (auto& [name, info] : shaders_) {
        if (info && info->module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, info->module, nullptr);
            info->module    = VK_NULL_HANDLE;
            info->is_loaded = false;
        }
    }
    shaders_.clear();
    return std::monostate{};
}

Result<std::monostate> ShaderManager::preload_cache(
        const std::filesystem::path& cache_dir) {
    cache_dir_ = cache_dir;
    return std::monostate{};
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

Result<std::monostate> ShaderManager::create_shader_module(
        ShaderInfo& info, const std::vector<uint32_t>& spirv) {
    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = spirv.size() * sizeof(uint32_t);
    ci.pCode    = spirv.data();

    VkResult r = vkCreateShaderModule(device_, &ci, nullptr, &info.module);
    if (r != VK_SUCCESS) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "vkCreateShaderModule failed for '" + info.name + "' (VkResult=" +
            std::to_string(static_cast<int>(r)) + ")"
        });
    }
    info.is_loaded = true;
    return std::monostate{};
}

Result<CompilationResult> ShaderManager::compile_shader(
        const std::filesystem::path& source_path, VkShaderStageFlagBits stage) {
    return compile_with_dxc(source_path, stage).and_then(
        [&](std::vector<uint32_t> spirv) -> Result<CompilationResult> {
            CompilationResult cr;
            cr.success      = true;
            cr.spirv_code   = std::move(spirv);
            cr.content_hash = fnv1a_hex(
                reinterpret_cast<const uint8_t*>(cr.spirv_code.data()),
                cr.spirv_code.size() * 4);
            return cr;
        });
}

Result<std::vector<uint32_t>> ShaderManager::compile_with_dxc(
        const std::filesystem::path& source_path, VkShaderStageFlagBits stage) {
    // Resolve DXC path (injected by cmake/shader_compiler.cmake as GW_DXC_PATH).
#ifdef GW_DXC_PATH
    const std::string dxc = GW_DXC_PATH;
#else
    const std::string dxc = "dxc";
#endif

    const std::string target    = get_dxc_target(stage);
    const std::string entry     = get_dxc_entry_point(source_path);
    const std::string out_path  = (source_path.parent_path() /
                                    (source_path.stem().string() + ".spv")).string();

    const std::string cmd = dxc + " -spirv -T " + target +
                            " -E " + entry +
                            " -Fo " + out_path +
                            " " + source_path.string();

    const int rc = std::system(cmd.c_str());  // NOLINT(cert-env33-c)
    if (rc != 0) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "DXC compilation failed (rc=" + std::to_string(rc) + "): " + cmd
        });
    }

    // Read generated SPIR-V
    std::ifstream f(out_path, std::ios::ate | std::ios::binary);
    if (!f) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "DXC wrote no output at: " + out_path
        });
    }
    const size_t sz = static_cast<size_t>(f.tellg());
    std::vector<uint32_t> spirv(sz / 4);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(spirv.data()), static_cast<std::streamsize>(sz));
    return spirv;
}

std::string ShaderManager::get_dxc_target(VkShaderStageFlagBits stage) {
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:                  return "vs_6_6";
        case VK_SHADER_STAGE_FRAGMENT_BIT:                return "ps_6_6";
        case VK_SHADER_STAGE_COMPUTE_BIT:                 return "cs_6_6";
        case VK_SHADER_STAGE_GEOMETRY_BIT:                return "gs_6_6";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return "hs_6_6";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "ds_6_6";
        default: return "vs_6_6";
    }
}

std::string ShaderManager::get_dxc_entry_point(const std::filesystem::path& /*source*/) {
    return "main";
}

VkShaderStageFlagBits ShaderManager::get_stage_from_extension(
        const std::filesystem::path& path) {
    const std::string ext = path.extension().string();
    if (ext == ".vert" || path.stem().extension() == ".vert") return VK_SHADER_STAGE_VERTEX_BIT;
    if (ext == ".frag" || path.stem().extension() == ".frag") return VK_SHADER_STAGE_FRAGMENT_BIT;
    if (ext == ".comp" || path.stem().extension() == ".comp") return VK_SHADER_STAGE_COMPUTE_BIT;
    if (ext == ".geom" || path.stem().extension() == ".geom") return VK_SHADER_STAGE_GEOMETRY_BIT;
    return VK_SHADER_STAGE_VERTEX_BIT;
}

std::string ShaderManager::calculate_content_hash(const std::string& content) {
    return fnv1a_hex(reinterpret_cast<const uint8_t*>(content.data()), content.size());
}

std::filesystem::path ShaderManager::get_cache_path(
        const std::string& name, VkShaderStageFlagBits /*stage*/) {
    return cache_dir_ / (name + ".spv");
}

Result<std::monostate> ShaderManager::save_to_cache(
        const std::string& name, VkShaderStageFlagBits stage,
        const std::vector<uint32_t>& spirv, const std::string& /*hash*/) {
    const auto path = get_cache_path(name, stage);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::CompilationFailed,
            "Cannot write cache file: " + path.string()
        });
    }
    f.write(reinterpret_cast<const char*>(spirv.data()),
            static_cast<std::streamsize>(spirv.size() * 4));
    return std::monostate{};
}

Result<std::vector<uint32_t>> ShaderManager::load_from_cache(
        const std::string& name, VkShaderStageFlagBits stage,
        const std::string& /*expected_hash*/) {
    const auto path = get_cache_path(name, stage);
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidResourceHandle,
            "Cache miss for: " + name
        });
    }
    const size_t sz = static_cast<size_t>(f.tellg());
    std::vector<uint32_t> spirv(sz / 4);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(spirv.data()), static_cast<std::streamsize>(sz));
    return spirv;
}

Result<std::monostate> ShaderManager::scan_directory() {
    if (!std::filesystem::exists(watch_directory_)) return std::monostate{};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(watch_directory_)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        if (p.extension() == ".hlsl") {
            const auto stage = get_stage_from_extension(p);
            const std::string name = p.stem().string();
            if (!shaders_.count(name)) {
                // Auto-register discovered shaders
                auto info = std::make_unique<ShaderInfo>(name, stage);
                info->source_path = p;
                shaders_[name] = std::move(info);
            }
        }
    }
    return std::monostate{};
}

bool ShaderManager::has_file_changed(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) return false;
    for (const auto& [name, mtime] : last_modified_times_) {
        auto it = shaders_.find(name);
        if (it == shaders_.end()) continue;
        if (it->second->source_path == file_path) {
            return std::filesystem::last_write_time(file_path) != mtime;
        }
    }
    return false;
}

Result<std::monostate> ShaderManager::reload_shader(const std::string& name) {
    auto it = shaders_.find(name);
    if (it == shaders_.end()) {
        return std::unexpected(FrameGraphError{
            FrameGraphErrorType::InvalidResourceHandle,
            "Shader not found for reload: " + name
        });
    }
    auto& info = *it->second;

    // Recompile from HLSL source
    auto cr = compile_shader(info.source_path, info.stage);
    if (!cr) return std::unexpected(cr.error());

    // Destroy old module
    if (info.module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, info.module, nullptr);
        info.module    = VK_NULL_HANDLE;
        info.is_loaded = false;
    }

    // Create new module
    info.content_hash = cr.value().content_hash;
    return create_shader_module(info, cr.value().spirv_code);
}

} // namespace gw::render::shader
