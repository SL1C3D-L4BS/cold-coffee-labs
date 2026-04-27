#pragma once

#include "../frame_graph/frame_graph.hpp"
#include "../frame_graph/error.hpp"
#include "../forward_plus/cluster.hpp"
#include "../shadows/cascaded_shadows.hpp"
#include "../ibl/ibl.hpp"
#include "../hal/vulkan_device.hpp"
#include "engine/math/vec.hpp"
#include "engine/math/mat.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace gw::render::validation {

using gw::render::frame_graph::Result;
using gw::render::frame_graph::FrameGraphError;
using gw::math::Mat4f;
using gw::math::Vec3f;
using gw::math::Vec4f;

// Milestone validation configuration
struct MilestoneConfig {
    float    target_fps             = 60.0f;
    float    min_fps                = 55.0f;
    uint32_t target_resolution_x    = 1920;
    uint32_t target_resolution_y    = 1080;
    uint32_t num_lights             = 100;
    uint32_t num_objects            = 1000;
    float    scene_size             = 100.0f;
    float    max_frame_time_ms      = 16.67f;
    float    max_gpu_time_ms        = 15.0f;
    float    max_cpu_time_ms        = 2.0f;
    uint32_t max_memory_usage_mb    = 2048;
    bool     validate_frame_graph   = true;
    bool     validate_forward_plus  = true;
    bool     validate_cascaded_shadows = true;
    bool     validate_ibl           = true;
    bool     validate_async_compute = true;
};

// Performance metrics
struct PerformanceMetrics {
    float    average_fps      = 0.0f;
    float    min_fps          = 0.0f;
    float    max_fps          = 0.0f;
    float    frame_time_ms    = 0.0f;
    float    gpu_time_ms      = 0.0f;
    float    cpu_time_ms      = 0.0f;
    uint64_t memory_usage_bytes = 0;
    uint32_t frame_count      = 0;

    std::vector<float> frame_times;
    std::vector<float> gpu_times;
    std::vector<float> cpu_times;

    void reset();
    void update(float frame_time, float gpu_time, float cpu_time);
};

// Validation result
struct ValidationResult {
    bool        passed = false;
    std::string test_name;
    std::string error_message;
    PerformanceMetrics metrics;

    struct Features {
        bool frame_graph_compiled       = false;
        bool forward_plus_rendered      = false;
        bool cascaded_shadows_rendered  = false;
        bool ibl_rendered               = false;
        bool async_compute_working      = false;
    } features;

    void reset();
};

// Test scene generator
class TestSceneGenerator {
public:
    struct TestObject {
        Mat4f    transform;
        Vec3f    albedo;
        float    metallic   = 0.0f;
        float    roughness  = 0.5f;
        uint32_t mesh_id    = 0;
    };

    struct TestLight {
        Vec3f    position;
        Vec3f    color;
        float    intensity   = 100.0f;
        float    radius      = 10.0f;
        uint32_t type        = 0;   // 0=point, 1=spot, 2=directional
        Vec3f    direction;
        float    inner_cone  = 0.0f;
        float    outer_cone  = 0.0f;
    };

    static std::vector<TestObject> generate_objects(uint32_t count, float scene_size);
    static std::vector<TestLight>  generate_lights(uint32_t count, float scene_size);

private:
    static Mat4f make_translation(float x, float y, float z);
    static Vec3f random_color(float lo = 0.2f, float hi = 0.9f);
};

// Milestone validator
class MilestoneValidator {
public:
    explicit MilestoneValidator(hal::VulkanDevice& device);
    ~MilestoneValidator();

    MilestoneValidator(const MilestoneValidator&) = delete;
    MilestoneValidator& operator=(const MilestoneValidator&) = delete;
    MilestoneValidator(MilestoneValidator&&)            = delete;
    MilestoneValidator& operator=(MilestoneValidator&&) = delete;

    Result<std::monostate> initialize(const MilestoneConfig& config);

    ValidationResult validate_foundation_renderer();
    ValidationResult test_frame_graph();
    ValidationResult test_forward_plus_lighting();
    ValidationResult test_cascaded_shadows();
    ValidationResult test_ibl_system();
    ValidationResult test_async_compute();
    ValidationResult test_performance();

    Result<std::monostate> run_performance_benchmark(uint32_t duration_seconds = 60);
    Result<std::monostate> validate_memory_usage();

    [[nodiscard]] const PerformanceMetrics& metrics() const { return metrics_; }
    [[nodiscard]] const MilestoneConfig&    config()  const { return config_; }

private:
    hal::VulkanDevice& device_;
    MilestoneConfig    config_;
    PerformanceMetrics metrics_;

    std::unique_ptr<frame_graph::FrameGraph>         frame_graph_;
    std::unique_ptr<forward_plus::ClusteredLighting> forward_plus_;
    std::unique_ptr<shadows::CascadedShadowMapper>   shadow_mapper_;
    std::unique_ptr<ibl::IBLSystem>                  ibl_system_;

    std::vector<TestSceneGenerator::TestObject> test_objects_;
    std::vector<TestSceneGenerator::TestLight>  test_lights_;

    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;

    Result<std::monostate> setup_test_scene();
    Result<std::monostate> cleanup_test_scene();
    Result<std::monostate> render_test_frame();

    void begin_frame_timing();
    void end_frame_timing();

    bool validate_frame_graph_compilation();

    std::string generate_validation_report(const ValidationResult& result) const;
    std::string generate_performance_report() const;
};

// Utility functions
namespace utils {
    bool meets_performance_target(const PerformanceMetrics& metrics,
                                  const MilestoneConfig& config);

    struct MemoryUsage {
        uint64_t total_allocated = 0;
        uint64_t total_used      = 0;
        uint64_t gpu_usage       = 0;
        uint64_t cpu_usage       = 0;
    };

    MemoryUsage get_memory_usage(hal::VulkanDevice& device);
    bool validate_frame_graph_consistency(const frame_graph::FrameGraph& frame_graph);
}

} // namespace gw::render::validation
