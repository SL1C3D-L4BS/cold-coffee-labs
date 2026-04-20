#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../forward_plus/cluster.hpp"
#include "../shadows/cascaded_shadows.hpp"
#include "../ibl/ibl.hpp"
#include "engine/core/math.hpp"
#include "engine/core/result.hpp"
#include <vector>
#include <chrono>
#include <memory>

namespace cold_coffee::engine::render::validation {

// Milestone validation configuration
struct MilestoneConfig {
    // Performance targets
    float target_fps = 60.0f;
    float min_fps = 55.0f;
    uint32_t target_resolution_x = 1920;
    uint32_t target_resolution_y = 1080;
    
    // Test scene configuration
    uint32_t num_lights = 100;
    uint32_t num_objects = 1000;
    float scene_size = 100.0f;
    
    // Validation thresholds
    float max_frame_time_ms = 16.67f; // 60 FPS
    float max_gpu_time_ms = 15.0f;
    float max_cpu_time_ms = 2.0f;
    uint32_t max_memory_usage_mb = 2048;
    
    // Feature validation
    bool validate_frame_graph = true;
    bool validate_forward_plus = true;
    bool validate_cascaded_shadows = true;
    bool validate_ibl = true;
    bool validate_async_compute = true;
};

// Performance metrics
struct PerformanceMetrics {
    float average_fps = 0.0f;
    float min_fps = 0.0f;
    float max_fps = 0.0f;
    float frame_time_ms = 0.0f;
    float gpu_time_ms = 0.0f;
    float cpu_time_ms = 0.0f;
    uint64_t memory_usage_bytes = 0;
    uint32_t frame_count = 0;
    
    // Frame time history
    std::vector<float> frame_times;
    std::vector<float> gpu_times;
    std::vector<float> cpu_times;
    
    void reset() {
        average_fps = 0.0f;
        min_fps = 0.0f;
        max_fps = 0.0f;
        frame_time_ms = 0.0f;
        gpu_time_ms = 0.0f;
        cpu_time_ms = 0.0f;
        memory_usage_bytes = 0;
        frame_count = 0;
        frame_times.clear();
        gpu_times.clear();
        cpu_times.clear();
    }
    
    void update(float frame_time, float gpu_time, float cpu_time) {
        frame_times.push_back(frame_time);
        gpu_times.push_back(gpu_time);
        cpu_times.push_back(cpu_time);
        
        // Keep only last 60 frames for rolling average
        if (frame_times.size() > 60) {
            frame_times.erase(frame_times.begin());
            gpu_times.erase(gpu_times.begin());
            cpu_times.erase(cpu_times.begin());
        }
        
        // Calculate averages
        float total_frame_time = 0.0f;
        float total_gpu_time = 0.0f;
        float total_cpu_time = 0.0f;
        
        for (size_t i = 0; i < frame_times.size(); ++i) {
            total_frame_time += frame_times[i];
            total_gpu_time += gpu_times[i];
            total_cpu_time += cpu_times[i];
        }
        
        frame_time_ms = total_frame_time / frame_times.size();
        gpu_time_ms = total_gpu_time / gpu_times.size();
        cpu_time_ms = total_cpu_time / cpu_times.size();
        
        average_fps = 1000.0f / frame_time_ms;
        min_fps = 1000.0f / (*std::max_element(frame_times.begin(), frame_times.end()));
        max_fps = 1000.0f / (*std::min_element(frame_times.begin(), frame_times.end()));
        
        frame_count++;
    }
};

// Validation result
struct ValidationResult {
    bool passed = false;
    std::string test_name;
    std::string error_message;
    PerformanceMetrics metrics;
    
    // Feature-specific results
    struct {
        bool frame_graph_compiled = false;
        bool forward_plus_rendered = false;
        bool cascaded_shadows_rendered = false;
        bool ibl_rendered = false;
        bool async_compute_working = false;
    } features;
    
    void reset() {
        passed = false;
        test_name.clear();
        error_message.clear();
        metrics.reset();
        features = {};
    }
};

// Test scene generator
class TestSceneGenerator {
public:
    struct TestObject {
        glm::mat4 transform;
        glm::vec3 albedo;
        float metallic;
        float roughness;
        uint32_t mesh_id;
    };
    
    struct TestLight {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float radius;
        uint32_t type; // 0=point, 1=spot, 2=directional
        glm::vec3 direction;
        float inner_cone;
        float outer_cone;
    };
    
    static std::vector<TestObject> generate_objects(uint32_t count, float scene_size);
    static std::vector<TestLight> generate_lights(uint32_t count, float scene_size);
    static glm::mat4 create_random_transform(float scene_size);
    static glm::vec3 create_random_color();
};

// Milestone validator class
class MilestoneValidator {
public:
    MilestoneValidator(const class hal::Device& device);
    ~MilestoneValidator();
    
    // Non-copyable, movable
    MilestoneValidator(const MilestoneValidator&) = delete;
    MilestoneValidator& operator=(const MilestoneValidator&) = delete;
    MilestoneValidator(MilestoneValidator&&) = default;
    MilestoneValidator& operator=(MilestoneValidator&&) = default;
    
    // Initialize validator
    Result<void> initialize(const MilestoneConfig& config);
    
    // Run full milestone validation
    ValidationResult validate_foundation_renderer();
    
    // Individual feature tests
    ValidationResult test_frame_graph();
    ValidationResult test_forward_plus_lighting();
    ValidationResult test_cascaded_shadows();
    ValidationResult test_ibl_system();
    ValidationResult test_async_compute();
    ValidationResult test_performance();
    
    // Performance benchmarking
    Result<void> run_performance_benchmark(uint32_t duration_seconds = 60);
    
    // Memory validation
    Result<void> validate_memory_usage();
    
    // Getters
    const PerformanceMetrics& metrics() const { return metrics_; }
    const MilestoneConfig& config() const { return config_; }
    
private:
    const class hal::Device& device_;
    MilestoneConfig config_;
    PerformanceMetrics metrics_;
    
    // Rendering systems for testing
    std::unique_ptr<frame_graph::FrameGraph> frame_graph_;
    std::unique_ptr<forward_plus::ClusteredLighting> forward_plus_;
    std::unique_ptr<shadows::CascadedShadowMapper> shadow_mapper_;
    std::unique_ptr<ibl::IBLSystem> ibl_system_;
    
    // Test resources
    std::vector<TestSceneGenerator::TestObject> test_objects_;
    std::vector<TestSceneGenerator::TestLight> test_lights_;
    
    // Timing utilities
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    
    // Validation helpers
    Result<void> setup_test_scene();
    Result<void> cleanup_test_scene();
    Result<void> render_test_frame();
    
    // Performance measurement
    void begin_frame_timing();
    void end_frame_timing();
    float measure_gpu_time();
    float measure_cpu_time();
    
    // Feature validation helpers
    bool validate_frame_graph_compilation();
    bool validate_forward_plus_rendering();
    bool validate_shadow_mapping();
    bool validate_ibl_rendering();
    bool validate_async_compute_execution();
    
    // Report generation
    std::string generate_validation_report(const ValidationResult& result);
    std::string generate_performance_report();
    std::string generate_feature_report();
};

// Utility functions
namespace utils {
    // GPU timing utilities
    class GPUTimer {
    public:
        GPUTimer(const hal::Device& device);
        ~GPUTimer();
        
        void start();
        void stop();
        float get_time_ms();
        
    private:
        const hal::Device& device_;
        VkQueryPool query_pool_ = VK_NULL_HANDLE;
        bool started_ = false;
    };
    
    // Memory usage utilities
    struct MemoryUsage {
        uint64_t total_allocated = 0;
        uint64_t total_used = 0;
        uint64_t gpu_usage = 0;
        uint64_t cpu_usage = 0;
    };
    
    MemoryUsage get_memory_usage(const hal::Device& device);
    
    // Validation utilities
    bool validate_vulkan_result(VkResult result, const std::string& operation);
    bool validate_frame_graph_consistency(const frame_graph::FrameGraph& frame_graph);
    bool validate_shader_compilation(const std::string& shader_path);
    
    // Performance utilities
    float calculate_frame_time_variance(const std::vector<float>& frame_times);
    bool meets_performance_target(const PerformanceMetrics& metrics, const MilestoneConfig& config);
}

} // namespace cold_coffee::engine::render::validation
