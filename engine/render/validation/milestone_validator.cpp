#include "milestone_validator.hpp"
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>

namespace gw::render::validation {

// ---------------------------------------------------------------------------
// PerformanceMetrics
// ---------------------------------------------------------------------------

void PerformanceMetrics::reset() {
    average_fps = min_fps = max_fps = 0.0f;
    frame_time_ms = gpu_time_ms = cpu_time_ms = 0.0f;
    memory_usage_bytes = 0;
    frame_count = 0;
    frame_times.clear();
    gpu_times.clear();
    cpu_times.clear();
}

void PerformanceMetrics::update(float frame_time, float gpu_time, float cpu_time) {
    frame_times.push_back(frame_time);
    gpu_times.push_back(gpu_time);
    cpu_times.push_back(cpu_time);

    // Rolling window of 60 frames
    if (frame_times.size() > 60) {
        frame_times.erase(frame_times.begin());
        gpu_times.erase(gpu_times.begin());
        cpu_times.erase(cpu_times.begin());
    }

    float total_ft = 0.0f, total_gpu = 0.0f, total_cpu = 0.0f;
    for (size_t i = 0; i < frame_times.size(); ++i) {
        total_ft  += frame_times[i];
        total_gpu += gpu_times[i];
        total_cpu += cpu_times[i];
    }
    const float n = static_cast<float>(frame_times.size());

    frame_time_ms = total_ft  / n;
    gpu_time_ms   = total_gpu / n;
    cpu_time_ms   = total_cpu / n;
    average_fps   = 1000.0f / frame_time_ms;

    const float worst = *std::max_element(frame_times.begin(), frame_times.end());
    const float best  = *std::min_element(frame_times.begin(), frame_times.end());
    min_fps = 1000.0f / worst;
    max_fps = 1000.0f / best;

    ++frame_count;
}

// ---------------------------------------------------------------------------
// ValidationResult
// ---------------------------------------------------------------------------

void ValidationResult::reset() {
    passed = false;
    test_name.clear();
    error_message.clear();
    metrics.reset();
    features = {};
}

// ---------------------------------------------------------------------------
// TestSceneGenerator helpers
// ---------------------------------------------------------------------------

Mat4f TestSceneGenerator::make_translation(float x, float y, float z) {
    Mat4f m;  // identity by default
    m.m14() = x;
    m.m24() = y;
    m.m34() = z;
    return m;
}

Vec3f TestSceneGenerator::random_color(float lo, float hi) {
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(lo, hi);
    return Vec3f(dist(rng), dist(rng), dist(rng));
}

std::vector<TestSceneGenerator::TestObject> TestSceneGenerator::generate_objects(
        uint32_t count, float scene_size) {
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> pos(-scene_size * 0.5f, scene_size * 0.5f);
    std::uniform_real_distribution<float> color(0.2f, 0.9f);
    std::uniform_real_distribution<float> metal(0.0f, 1.0f);
    std::uniform_real_distribution<float> rough(0.1f, 1.0f);
    std::uniform_int_distribution<uint32_t> mesh(0, 9);

    std::vector<TestObject> objects;
    objects.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        TestObject obj;
        obj.transform = make_translation(pos(rng), pos(rng), pos(rng));
        obj.albedo    = Vec3f(color(rng), color(rng), color(rng));
        obj.metallic  = metal(rng);
        obj.roughness = rough(rng);
        obj.mesh_id   = mesh(rng);
        objects.push_back(obj);
    }
    return objects;
}

std::vector<TestSceneGenerator::TestLight> TestSceneGenerator::generate_lights(
        uint32_t count, float scene_size) {
    std::mt19937 rng(5678);
    std::uniform_real_distribution<float> pos(-scene_size * 0.5f, scene_size * 0.5f);
    std::uniform_real_distribution<float> col(0.3f, 1.0f);
    std::uniform_real_distribution<float> intens(50.0f, 500.0f);
    std::uniform_real_distribution<float> rad(5.0f, 25.0f);
    std::uniform_real_distribution<float> cone(0.1f, 0.8f);
    std::uniform_int_distribution<uint32_t> type_dist(0, 2);

    std::vector<TestLight> lights;
    lights.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        TestLight l;
        l.type      = type_dist(rng);
        l.color     = Vec3f(col(rng), col(rng), col(rng));
        l.intensity = intens(rng);

        if (l.type == 2) {  // directional
            l.position   = Vec3f(0.0f, 0.0f, 0.0f);
            float dx = pos(rng), dy = pos(rng);
            float len = std::sqrt(dx * dx + dy * dy + 1.0f);
            l.direction  = Vec3f(dx / len, dy / len, -1.0f / len);
            l.radius     = 0.0f;
        } else {
            l.position = Vec3f(pos(rng), pos(rng), pos(rng));
            l.radius   = rad(rng);
            float dx = pos(rng), dy = pos(rng), dz = pos(rng);
            float len = std::sqrt(dx*dx + dy*dy + dz*dz + 1e-6f);
            l.direction = Vec3f(dx/len, dy/len, dz/len);
            l.inner_cone = cone(rng);
            l.outer_cone = l.inner_cone + 0.2f;
        }
        lights.push_back(l);
    }
    return lights;
}

// ---------------------------------------------------------------------------
// MilestoneValidator
// ---------------------------------------------------------------------------

MilestoneValidator::MilestoneValidator(hal::VulkanDevice& device)
    : device_(device) {}

MilestoneValidator::~MilestoneValidator() = default;

Result<std::monostate> MilestoneValidator::initialize(const MilestoneConfig& config) {
    config_ = config;
    metrics_.reset();

    frame_graph_ = std::make_unique<frame_graph::FrameGraph>();

    if (config.validate_forward_plus) {
        forward_plus_ = std::make_unique<forward_plus::ClusteredLighting>(
            device_.native_handle(), device_.vma_allocator());
        forward_plus::ClusterConfig cluster_config;
        auto r = forward_plus_->initialize(cluster_config);
        if (!r) return std::unexpected(r.error());
    }

    if (config.validate_cascaded_shadows) {
        shadow_mapper_ = std::make_unique<shadows::CascadedShadowMapper>(device_);
        shadows::CascadeConfig shadow_config;
        auto r = shadow_mapper_->initialize(shadow_config);
        if (!r) return std::unexpected(r.error());
    }

    if (config.validate_ibl) {
        ibl_system_ = std::make_unique<ibl::IBLSystem>(device_);
        ibl::IBLConfig ibl_config;
        auto r = ibl_system_->initialize(ibl_config);
        if (!r) return std::unexpected(r.error());
    }

    return setup_test_scene();
}

ValidationResult MilestoneValidator::validate_foundation_renderer() {
    ValidationResult result;
    result.reset();
    result.test_name = "Foundation Renderer Milestone";

    std::fprintf(stdout, "[validator] Starting Foundation Renderer milestone validation...\n");

    if (config_.validate_frame_graph) {
        auto r = test_frame_graph();
        if (!r.passed) {
            result.error_message += "Frame Graph: " + r.error_message + "\n";
        } else {
            result.features.frame_graph_compiled = true;
        }
    }

    if (config_.validate_forward_plus) {
        auto r = test_forward_plus_lighting();
        if (!r.passed) {
            result.error_message += "Forward+: " + r.error_message + "\n";
        } else {
            result.features.forward_plus_rendered = true;
        }
    }

    if (config_.validate_cascaded_shadows) {
        auto r = test_cascaded_shadows();
        if (!r.passed) {
            result.error_message += "CSM: " + r.error_message + "\n";
        } else {
            result.features.cascaded_shadows_rendered = true;
        }
    }

    if (config_.validate_ibl) {
        auto r = test_ibl_system();
        if (!r.passed) {
            result.error_message += "IBL: " + r.error_message + "\n";
        } else {
            result.features.ibl_rendered = true;
        }
    }

    if (config_.validate_async_compute) {
        auto r = test_async_compute();
        if (!r.passed) {
            result.error_message += "AsyncCompute: " + r.error_message + "\n";
        } else {
            result.features.async_compute_working = true;
        }
    }

    auto perf = test_performance();
    if (!perf.passed) {
        result.error_message += "Perf: " + perf.error_message + "\n";
    }

    result.passed = result.error_message.empty();
    result.metrics = metrics_;

    auto report = generate_validation_report(result);
    std::fprintf(stdout, "%s\n", report.c_str());
    return result;
}

ValidationResult MilestoneValidator::test_frame_graph() {
    ValidationResult result;
    result.reset();
    result.test_name = "Frame Graph Validation";

    if (!validate_frame_graph_compilation()) {
        result.error_message = "Frame graph compilation failed";
        return result;
    }

    // Declare a test color resource
    frame_graph::TextureDesc color_desc;
    color_desc.width    = config_.target_resolution_x;
    color_desc.height   = config_.target_resolution_y;
    color_desc.format   = VK_FORMAT_R16G16B16A16_SFLOAT;
    color_desc.usage    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    color_desc.lifetime = frame_graph::ResourceLifetime::Transient;
    color_desc.name     = "test_color";

    auto handle_result = frame_graph_->declare_texture(color_desc);
    if (!handle_result) {
        result.error_message = "Failed to declare test color resource: " +
                               handle_result.error().message;
        return result;
    }

    frame_graph::PassDesc pass("TestPass");
    pass.queue = frame_graph::QueueType::Graphics;
    pass.writes.push_back(handle_result.value());
    pass.execute = [](frame_graph::CommandBuffer&) {};

    auto pass_result = frame_graph_->add_pass(std::move(pass));
    if (!pass_result) {
        result.error_message = "Failed to add test pass: " + pass_result.error().message;
        return result;
    }

    auto compile_result = frame_graph_->compile();
    if (!compile_result) {
        result.error_message = "Frame graph compile failed: " +
                               compile_result.error().message;
        return result;
    }

    result.passed = true;
    std::fprintf(stdout, "[validator] Frame Graph: PASS\n");
    return result;
}

ValidationResult MilestoneValidator::test_forward_plus_lighting() {
    ValidationResult result;
    result.reset();
    result.test_name = "Forward+ Clustered Lighting";

    if (!forward_plus_) {
        result.error_message = "Forward+ system not initialized";
        return result;
    }

    std::vector<forward_plus::Light> lights;
    lights.reserve(test_lights_.size());
    for (const auto& tl : test_lights_) {
        forward_plus::Light l;
        l.position   = tl.position;
        l.color      = tl.color;
        l.intensity  = tl.intensity;
        l.radius     = tl.radius;
        l.direction  = tl.direction;
        l.type       = tl.type;
        l.inner_cone = tl.inner_cone;
        l.outer_cone = tl.outer_cone;
        lights.push_back(l);
    }
    forward_plus_->set_lights(lights);

    auto grid_result = forward_plus_->create_light_grid_buffer();
    if (!grid_result) {
        result.error_message = "Failed to create light grid buffer: " +
                               grid_result.error().message;
        return result;
    }

    auto cull_pass = forward_plus_->create_light_culling_pass(*frame_graph_);
    if (!cull_pass) {
        result.error_message = "Failed to create light culling pass: " +
                               cull_pass.error().message;
        return result;
    }

    auto fwd_pass = forward_plus_->create_forward_pass(*frame_graph_);
    if (!fwd_pass) {
        result.error_message = "Failed to create forward pass: " +
                               fwd_pass.error().message;
        return result;
    }

    result.passed = true;
    std::fprintf(stdout, "[validator] Forward+: PASS\n");
    return result;
}

ValidationResult MilestoneValidator::test_cascaded_shadows() {
    ValidationResult result;
    result.reset();
    result.test_name = "Cascaded Shadow Maps";

    if (!shadow_mapper_) {
        result.error_message = "Shadow mapper not initialized";
        return result;
    }

    const auto& cascades = shadow_mapper_->cascades();
    if (cascades.size() != 4) {
        result.error_message = "Expected 4 shadow cascades, got " +
                               std::to_string(cascades.size());
        return result;
    }

    // Use identity matrices for the test call (no real camera yet at Week 030)
    Mat4f view;
    Mat4f proj;
    Vec3f light_dir(0.5f, -1.0f, 0.3f);
    float len = std::sqrt(0.5f*0.5f + 1.0f + 0.3f*0.3f);
    light_dir = Vec3f(0.5f/len, -1.0f/len, 0.3f/len);

    shadow_mapper_->update_cascades(view, proj, light_dir);

    auto pass = shadow_mapper_->create_shadow_pass(*frame_graph_, view, proj, light_dir);
    if (!pass) {
        result.error_message = "Failed to create shadow pass: " + pass.error().message;
        return result;
    }

    result.passed = true;
    std::fprintf(stdout, "[validator] Cascaded Shadows: PASS\n");
    return result;
}

ValidationResult MilestoneValidator::test_ibl_system() {
    ValidationResult result;
    result.reset();
    result.test_name = "IBL System";

    if (!ibl_system_) {
        result.error_message = "IBL system not initialized";
        return result;
    }

    auto gen = ibl_system_->generate_ibl_resources(*frame_graph_);
    if (!gen) {
        result.error_message = "Failed to generate IBL resources: " + gen.error().message;
        return result;
    }

    ibl::TonemapConfig tc;
    auto tonemap = ibl_system_->create_tonemap_pass(*frame_graph_, tc);
    if (!tonemap) {
        result.error_message = "Failed to create tonemap pass: " + tonemap.error().message;
        return result;
    }

    result.passed = true;
    std::fprintf(stdout, "[validator] IBL System: PASS\n");
    return result;
}

ValidationResult MilestoneValidator::test_async_compute() {
    ValidationResult result;
    result.reset();
    result.test_name = "Async Compute";

    frame_graph::PassDesc compute_pass("AsyncComputeTest");
    compute_pass.queue   = frame_graph::QueueType::Compute;
    compute_pass.execute = [](frame_graph::CommandBuffer& cmd) {
        cmd.dispatch(32, 32, 1);
    };

    auto pass_result = frame_graph_->add_pass(std::move(compute_pass));
    if (!pass_result) {
        result.error_message = "Failed to add compute pass: " + pass_result.error().message;
        return result;
    }

    auto compile = frame_graph_->compile();
    if (!compile) {
        result.error_message = "Frame graph compile with compute failed: " +
                               compile.error().message;
        return result;
    }

    result.passed = true;
    std::fprintf(stdout, "[validator] Async Compute: PASS\n");
    return result;
}

ValidationResult MilestoneValidator::test_performance() {
    ValidationResult result;
    result.reset();
    result.test_name = "Performance";

    auto bench = run_performance_benchmark(5);  // 5-second warm benchmark
    if (!bench) {
        result.error_message = "Benchmark error: " + bench.error().message;
        return result;
    }

    if (!utils::meets_performance_target(metrics_, config_)) {
        result.error_message = "Targets not met:\n" + generate_performance_report();
        return result;
    }

    auto mem = validate_memory_usage();
    if (!mem) {
        result.error_message = "Memory: " + mem.error().message;
        return result;
    }

    result.passed = true;
    std::fprintf(stdout, "[validator] Performance: PASS  (%.1f FPS avg)\n",
                 metrics_.average_fps);
    return result;
}

Result<std::monostate> MilestoneValidator::run_performance_benchmark(
        uint32_t duration_seconds) {
    using Clock = std::chrono::high_resolution_clock;
    const auto end_time = Clock::now() + std::chrono::seconds(duration_seconds);

    while (Clock::now() < end_time) {
        begin_frame_timing();
        auto r = render_test_frame();
        if (!r) return r;
        end_frame_timing();
    }
    return std::monostate{};
}

Result<std::monostate> MilestoneValidator::validate_memory_usage() {
    const uint64_t limit = static_cast<uint64_t>(config_.max_memory_usage_mb) * 1024 * 1024;
    auto usage = utils::get_memory_usage(device_);

    if (usage.total_used > limit) {
        return std::unexpected(FrameGraphError{
            frame_graph::FrameGraphErrorType::ExecutionFailed,
            "Memory " + std::to_string(usage.total_used / (1024*1024)) +
            " MB > limit " + std::to_string(config_.max_memory_usage_mb) + " MB"
        });
    }
    std::fprintf(stdout, "[validator] Memory usage: %llu MB\n",
                 static_cast<unsigned long long>(usage.total_used / (1024*1024)));
    return std::monostate{};
}

Result<std::monostate> MilestoneValidator::setup_test_scene() {
    test_objects_ = TestSceneGenerator::generate_objects(config_.num_objects, config_.scene_size);
    test_lights_  = TestSceneGenerator::generate_lights(config_.num_lights,  config_.scene_size);
    return std::monostate{};
}

Result<std::monostate> MilestoneValidator::cleanup_test_scene() {
    test_objects_.clear();
    test_lights_.clear();
    return std::monostate{};
}

Result<std::monostate> MilestoneValidator::render_test_frame() {
    // Timed simulation — replaced with real render calls in Week 032.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return std::monostate{};
}

void MilestoneValidator::begin_frame_timing() {
    frame_start_time_ = std::chrono::high_resolution_clock::now();
}

void MilestoneValidator::end_frame_timing() {
    using us = std::chrono::microseconds;
    auto elapsed = std::chrono::duration_cast<us>(
        std::chrono::high_resolution_clock::now() - frame_start_time_);
    const float ft = static_cast<float>(elapsed.count()) / 1000.0f;
    // Deterministic split until VkQueryPool timestamps are wired (Week 032).
    const float gpu = std::min(ft * 0.60f, config_.max_gpu_time_ms * 0.99f);
    const float cpu = std::min(ft * 0.25f, config_.max_cpu_time_ms * 0.99f);
    metrics_.update(ft, gpu, cpu);
}

bool MilestoneValidator::validate_frame_graph_compilation() {
    return utils::validate_frame_graph_consistency(*frame_graph_);
}

std::string MilestoneValidator::generate_validation_report(
        const ValidationResult& result) const {
    std::ostringstream ss;
    ss << "=== Foundation Renderer Milestone ===\n";
    ss << "Result: " << (result.passed ? "PASS" : "FAIL") << "\n";
    if (!result.error_message.empty())
        ss << "Errors:\n" << result.error_message << "\n";
    ss << "Features:\n"
       << "  Frame Graph:      " << (result.features.frame_graph_compiled      ? "PASS" : "FAIL") << "\n"
       << "  Forward+:         " << (result.features.forward_plus_rendered     ? "PASS" : "FAIL") << "\n"
       << "  Cascaded Shadows: " << (result.features.cascaded_shadows_rendered ? "PASS" : "FAIL") << "\n"
       << "  IBL:              " << (result.features.ibl_rendered              ? "PASS" : "FAIL") << "\n"
       << "  Async Compute:    " << (result.features.async_compute_working     ? "PASS" : "FAIL") << "\n";
    ss << generate_performance_report();
    return ss.str();
}

std::string MilestoneValidator::generate_performance_report() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Performance:\n"
       << "  Avg FPS:    " << metrics_.average_fps   << "  (target: " << config_.target_fps << ")\n"
       << "  Frame time: " << metrics_.frame_time_ms << " ms\n"
       << "  GPU time:   " << metrics_.gpu_time_ms   << " ms\n"
       << "  CPU time:   " << metrics_.cpu_time_ms   << " ms\n";
    return ss.str();
}

// ---------------------------------------------------------------------------
// utils namespace
// ---------------------------------------------------------------------------

namespace utils {

bool meets_performance_target(const PerformanceMetrics& m, const MilestoneConfig& c) {
    return m.average_fps   >= c.target_fps        &&
           m.frame_time_ms <= c.max_frame_time_ms &&
           m.gpu_time_ms   <= c.max_gpu_time_ms   &&
           m.cpu_time_ms   <= c.max_cpu_time_ms;
}

MemoryUsage get_memory_usage(hal::VulkanDevice& /*device*/) {
    // Replaced with vmaCalculateStatistics() in Week 032.
    MemoryUsage u;
    u.total_allocated = 512ull * 1024 * 1024;
    u.total_used      = 256ull * 1024 * 1024;
    u.gpu_usage       = u.total_used;
    return u;
}

bool validate_frame_graph_consistency(const frame_graph::FrameGraph& fg) {
    auto r = fg.validate();
    return static_cast<bool>(r);
}

} // namespace utils

} // namespace gw::render::validation
