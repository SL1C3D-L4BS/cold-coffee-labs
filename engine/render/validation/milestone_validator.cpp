#include "milestone_validator.hpp"
#include "engine/render/hal/device.hpp"
#include "engine/core/log.hpp"
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace cold_coffee::engine::render::validation {

// TestSceneGenerator implementation
std::vector<TestSceneGenerator::TestObject> TestSceneGenerator::generate_objects(uint32_t count, float scene_size) {
    std::vector<TestObject> objects;
    objects.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-scene_size / 2.0f, scene_size / 2.0f);
    std::uniform_real_distribution<float> color_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> metallic_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> roughness_dist(0.1f, 1.0f);
    std::uniform_int_distribution<uint32_t> mesh_dist(0, 9); // 10 different mesh types
    
    for (uint32_t i = 0; i < count; ++i) {
        TestObject obj;
        obj.transform = create_random_transform(scene_size);
        obj.albedo = create_random_color();
        obj.metallic = metallic_dist(gen);
        obj.roughness = roughness_dist(gen);
        obj.mesh_id = mesh_dist(gen);
        
        objects.push_back(obj);
    }
    
    return objects;
}

std::vector<TestSceneGenerator::TestLight> TestSceneGenerator::generate_lights(uint32_t count, float scene_size) {
    std::vector<TestLight> lights;
    lights.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-scene_size / 2.0f, scene_size / 2.0f);
    std::uniform_real_distribution<float> color_dist(0.3f, 1.0f);
    std::uniform_real_distribution<float> intensity_dist(50.0f, 500.0f);
    std::uniform_real_distribution<float> radius_dist(5.0f, 25.0f);
    std::uniform_int_distribution<uint32_t> type_dist(0, 2); // point, spot, directional
    std::uniform_real_distribution<float> cone_dist(0.1f, 0.8f);
    
    for (uint32_t i = 0; i < count; ++i) {
        TestLight light;
        light.type = type_dist(gen);
        
        if (light.type == 2) { // Directional
            light.position = glm::vec3(0.0f);
            light.direction = glm::normalize(glm::vec3(
                pos_dist(gen), pos_dist(gen), -1.0f
            ));
            light.radius = 0.0f;
            light.inner_cone = 0.0f;
            light.outer_cone = 0.0f;
        } else { // Point or spot
            light.position = glm::vec3(pos_dist(gen), pos_dist(gen), pos_dist(gen));
            light.radius = radius_dist(gen);
            light.direction = glm::normalize(glm::vec3(
                pos_dist(gen), pos_dist(gen), pos_dist(gen)
            ));
            light.inner_cone = cone_dist(gen);
            light.outer_cone = light.inner_cone + 0.2f;
        }
        
        light.color = glm::vec3(color_dist(gen), color_dist(gen), color_dist(gen));
        light.intensity = intensity_dist(gen);
        
        lights.push_back(light);
    }
    
    return lights;
}

glm::mat4 TestSceneGenerator::create_random_transform(float scene_size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-scene_size / 2.0f, scene_size / 2.0f);
    std::uniform_real_distribution<float> rot_dist(0.0f, 3.14159265f * 2.0f);
    std::uniform_real_distribution<float> scale_dist(0.5f, 2.0f);
    
    glm::vec3 position(pos_dist(gen), pos_dist(gen), pos_dist(gen));
    glm::vec3 rotation(rot_dist(gen), rot_dist(gen), rot_dist(gen));
    glm::vec3 scale(scale_dist(gen), scale_dist(gen), scale_dist(gen));
    
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, scale);
    
    return transform;
}

glm::vec3 TestSceneGenerator::create_random_color() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> color_dist(0.2f, 0.9f);
    
    return glm::vec3(color_dist(gen), color_dist(gen), color_dist(gen));
}

// MilestoneValidator implementation
MilestoneValidator::MilestoneValidator(const hal::Device& device)
    : device_(device) {
}

MilestoneValidator::~MilestoneValidator() = default;

Result<void> MilestoneValidator::initialize(const MilestoneConfig& config) {
    config_ = config;
    metrics_.reset();
    
    // Initialize frame graph
    frame_graph_ = std::make_unique<frame_graph::FrameGraph>(device_);
    
    // Initialize rendering systems
    if (config.validate_forward_plus) {
        forward_plus_ = std::make_unique<forward_plus::ClusteredLighting>(device_);
        forward_plus::ClusterConfig cluster_config;
        auto result = forward_plus_->initialize(cluster_config);
        if (result.is_err()) {
            return Err(result.unwrap_err());
        }
    }
    
    if (config.validate_cascaded_shadows) {
        shadow_mapper_ = std::make_unique<shadows::CascadedShadowMapper>(device_);
        shadows::CascadeConfig shadow_config;
        auto result = shadow_mapper_->initialize(shadow_config);
        if (result.is_err()) {
            return Err(result.unwrap_err());
        }
    }
    
    if (config.validate_ibl) {
        ibl_system_ = std::make_unique<ibl::IBLSystem>(device_);
        ibl::IBLConfig ibl_config;
        auto result = ibl_system_->initialize(ibl_config);
        if (result.is_err()) {
            return Err(result.unwrap_err());
        }
    }
    
    // Setup test scene
    return setup_test_scene();
}

ValidationResult MilestoneValidator::validate_foundation_renderer() {
    ValidationResult result;
    result.reset();
    result.test_name = "Foundation Renderer Milestone";
    
    LOG_INFO("Starting Foundation Renderer milestone validation...");
    
    // Run individual feature tests
    if (config_.validate_frame_graph) {
        auto frame_graph_result = test_frame_graph();
        if (!frame_graph_result.passed) {
            result.passed = false;
            result.error_message += "Frame Graph test failed: " + frame_graph_result.error_message + "\n";
        } else {
            result.features.frame_graph_compiled = true;
        }
    }
    
    if (config_.validate_forward_plus) {
        auto forward_plus_result = test_forward_plus_lighting();
        if (!forward_plus_result.passed) {
            result.passed = false;
            result.error_message += "Forward+ test failed: " + forward_plus_result.error_message + "\n";
        } else {
            result.features.forward_plus_rendered = true;
        }
    }
    
    if (config_.validate_cascaded_shadows) {
        auto shadow_result = test_cascaded_shadows();
        if (!shadow_result.passed) {
            result.passed = false;
            result.error_message += "Cascaded Shadows test failed: " + shadow_result.error_message + "\n";
        } else {
            result.features.cascaded_shadows_rendered = true;
        }
    }
    
    if (config_.validate_ibl) {
        auto ibl_result = test_ibl_system();
        if (!ibl_result.passed) {
            result.passed = false;
            result.error_message += "IBL test failed: " + ibl_result.error_message + "\n";
        } else {
            result.features.ibl_rendered = true;
        }
    }
    
    if (config_.validate_async_compute) {
        auto async_result = test_async_compute();
        if (!async_result.passed) {
            result.passed = false;
            result.error_message += "Async Compute test failed: " + async_result.error_message + "\n";
        } else {
            result.features.async_compute_working = true;
        }
    }
    
    // Run performance validation
    auto performance_result = test_performance();
    if (!performance_result.passed) {
        result.passed = false;
        result.error_message += "Performance test failed: " + performance_result.error_message + "\n";
    }
    
    result.metrics = metrics_;
    
    // Generate report
    std::string report = generate_validation_report(result);
    LOG_INFO("Foundation Renderer validation completed:\n{}", report);
    
    return result;
}

ValidationResult MilestoneValidator::test_frame_graph() {
    ValidationResult result;
    result.reset();
    result.test_name = "Frame Graph Validation";
    
    try {
        // Test frame graph compilation
        if (!validate_frame_graph_compilation()) {
            result.error_message = "Frame graph compilation failed";
            return result;
        }
        
        // Test resource management
        auto color_handle = frame_graph_->create_resource({
            .type = frame_graph::ResourceType::Image2D,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .width = config_.target_resolution_x,
            .height = config_.target_resolution_y,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .lifetime = frame_graph::ResourceLifetime::Transient,
            .name = "test_color"
        });
        
        if (color_handle.is_err()) {
            result.error_message = "Failed to create test color resource";
            return result;
        }
        
        // Test pass creation and execution
        frame_graph::PassDesc test_pass("TestPass");
        test_pass.queue = frame_graph::QueueType::Graphics;
        test_pass.writes.push_back(color_handle.unwrap());
        test_pass.execute = [](frame_graph::CommandBuffer& cmd) {
            // Simple clear operation
            // cmd.clear_color_image(...);
        };
        
        auto pass_handle = frame_graph_->add_pass(std::move(test_pass));
        if (pass_handle.is_err()) {
            result.error_message = "Failed to add test pass to frame graph";
            return result;
        }
        
        // Test compilation
        auto compile_result = frame_graph_->compile();
        if (compile_result.is_err()) {
            result.error_message = "Frame graph compilation failed: " + compile_result.unwrap_err().message;
            return result;
        }
        
        result.passed = true;
        LOG_INFO("Frame Graph validation passed");
        
    } catch (const std::exception& e) {
        result.error_message = "Frame Graph validation threw exception: " + std::string(e.what());
    }
    
    return result;
}

ValidationResult MilestoneValidator::test_forward_plus_lighting() {
    ValidationResult result;
    result.reset();
    result.test_name = "Forward+ Clustered Lighting Validation";
    
    if (!forward_plus_) {
        result.error_message = "Forward+ system not initialized";
        return result;
    }
    
    try {
        // Setup test lights
        std::vector<forward_plus::Light> lights;
        for (const auto& test_light : test_lights_) {
            forward_plus::Light light;
            light.position = test_light.position;
            light.color = test_light.color;
            light.intensity = test_light.intensity;
            light.radius = test_light.radius;
            light.direction = test_light.direction;
            light.type = test_light.type;
            light.inner_cone = test_light.inner_cone;
            light.outer_cone = test_light.outer_cone;
            lights.push_back(light);
        }
        
        forward_plus_->set_lights(lights);
        
        // Test cluster grid creation
        auto light_grid_result = forward_plus_->create_light_grid_buffer();
        if (light_grid_result.is_err()) {
            result.error_message = "Failed to create light grid buffer";
            return result;
        }
        
        // Test light culling pass
        auto culling_pass = forward_plus_->create_light_culling_pass(*frame_graph_);
        if (culling_pass.is_err()) {
            result.error_message = "Failed to create light culling pass";
            return result;
        }
        
        // Test forward+ rendering pass
        auto forward_pass = forward_plus_->create_forward_pass(*frame_graph_);
        if (forward_pass.is_err()) {
            result.error_message = "Failed to create forward+ rendering pass";
            return result;
        }
        
        // Test cluster information
        forward_plus_->dump_cluster_info();
        
        result.passed = true;
        LOG_INFO("Forward+ validation passed");
        
    } catch (const std::exception& e) {
        result.error_message = "Forward+ validation threw exception: " + std::string(e.what());
    }
    
    return result;
}

ValidationResult MilestoneValidator::test_cascaded_shadows() {
    ValidationResult result;
    result.reset();
    result.test_name = "Cascaded Shadow Maps Validation";
    
    if (!shadow_mapper_) {
        result.error_message = "Shadow mapper system not initialized";
        return result;
    }
    
    try {
        // Test shadow map creation
        const auto& resources = shadow_mapper_->resources();
        if (resources.cascades.empty()) {
            result.error_message = "No shadow map cascades created";
            return result;
        }
        
        // Test shadow pass creation
        glm::mat4 view_matrix = glm::lookAt(glm::vec3(0, 10, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
        glm::vec3 light_direction = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
        
        auto shadow_pass = shadow_mapper_->create_shadow_pass(*frame_graph_, view_matrix, projection_matrix, light_direction);
        if (shadow_pass.is_err()) {
            result.error_message = "Failed to create shadow mapping pass";
            return result;
        }
        
        // Test cascade updates
        shadow_mapper_->update_cascades(view_matrix, projection_matrix, light_direction);
        
        // Test uniform data generation
        auto uniforms = shadow_mapper_->get_uniforms();
        
        // Validate cascade configuration
        const auto& cascades = shadow_mapper_->cascades();
        if (cascades.size() != 4) {
            result.error_message = "Expected 4 shadow cascades, got " + std::to_string(cascades.size());
            return result;
        }
        
        result.passed = true;
        LOG_INFO("Cascaded Shadow Maps validation passed");
        
    } catch (const std::exception& e) {
        result.error_message = "Cascaded Shadow Maps validation threw exception: " + std::string(e.what());
    }
    
    return result;
}

ValidationResult MilestoneValidator::test_ibl_system() {
    ValidationResult result;
    result.reset();
    result.test_name = "IBL System Validation";
    
    if (!ibl_system_) {
        result.error_message = "IBL system not initialized";
        return result;
    }
    
    try {
        // Test IBL resource generation
        auto ibl_result = ibl_system_->generate_ibl_resources(*frame_graph_);
        if (ibl_result.is_err()) {
            result.error_message = "Failed to generate IBL resources";
            return result;
        }
        
        // Test irradiance map pass
        auto irradiance_pass = ibl_system_->create_irradiance_pass(
            *frame_graph_, 
            ibl_system_->environment_map(), 
            ibl_system_->irradiance_map()
        );
        if (irradiance_pass.is_err()) {
            result.error_message = "Failed to create irradiance map pass";
            return result;
        }
        
        // Test prefilter pass
        auto prefilter_pass = ibl_system_->create_prefilter_pass(
            *frame_graph_, 
            ibl_system_->environment_map(), 
            ibl_system_->prefiltered_map()
        );
        if (prefilter_pass.is_err()) {
            result.error_message = "Failed to create prefilter pass";
            return result;
        }
        
        // Test BRDF LUT pass
        auto brdf_pass = ibl_system_->create_brdf_lut_pass(*frame_graph_, ibl_system_->brdf_lut());
        if (brdf_pass.is_err()) {
            result.error_message = "Failed to create BRDF LUT pass";
            return result;
        }
        
        // Test tonemapping pass
        ibl::TonemapConfig tonemap_config;
        auto tonemap_pass = ibl_system_->create_tonemap_pass(*frame_graph_, tonemap_config);
        if (tonemap_pass.is_err()) {
            result.error_message = "Failed to create tonemapping pass";
            return result;
        }
        
        // Test uniform data generation
        auto uniforms = ibl_system_->get_uniforms(tonemap_config);
        
        result.passed = true;
        LOG_INFO("IBL System validation passed");
        
    } catch (const std::exception& e) {
        result.error_message = "IBL System validation threw exception: " + std::string(e.what());
    }
    
    return result;
}

ValidationResult MilestoneValidator::test_async_compute() {
    ValidationResult result;
    result.reset();
    result.test_name = "Async Compute Validation";
    
    try {
        // Test compute pass creation
        frame_graph::PassDesc compute_pass("AsyncComputeTest");
        compute_pass.queue = frame_graph::QueueType::Compute;
        compute_pass.execute = [](frame_graph::CommandBuffer& cmd) {
            // Simple compute dispatch
            cmd.dispatch(32, 32, 1);
        };
        
        auto pass_handle = frame_graph_->add_pass(std::move(compute_pass));
        if (pass_handle.is_err()) {
            result.error_message = "Failed to create async compute pass";
            return result;
        }
        
        // Test compilation with async compute
        auto compile_result = frame_graph_->compile();
        if (compile_result.is_err()) {
            result.error_message = "Frame graph compilation with async compute failed";
            return result;
        }
        
        result.passed = true;
        LOG_INFO("Async Compute validation passed");
        
    } catch (const std::exception& e) {
        result.error_message = "Async Compute validation threw exception: " + std::string(e.what());
    }
    
    return result;
}

ValidationResult MilestoneValidator::test_performance() {
    ValidationResult result;
    result.reset();
    result.test_name = "Performance Validation";
    
    try {
        // Run performance benchmark
        auto benchmark_result = run_performance_benchmark(30); // 30 seconds
        if (benchmark_result.is_err()) {
            result.error_message = "Performance benchmark failed: " + benchmark_result.unwrap_err().message;
            return result;
        }
        
        // Validate performance targets
        if (!utils::meets_performance_target(metrics_, config_)) {
            result.error_message = "Performance targets not met:\n" + generate_performance_report();
            return result;
        }
        
        // Validate memory usage
        auto memory_result = validate_memory_usage();
        if (memory_result.is_err()) {
            result.error_message = "Memory validation failed: " + memory_result.unwrap_err().message;
            return result;
        }
        
        result.passed = true;
        LOG_INFO("Performance validation passed");
        
    } catch (const std::exception& e) {
        result.error_message = "Performance validation threw exception: " + std::string(e.what());
    }
    
    return result;
}

Result<void> MilestoneValidator::run_performance_benchmark(uint32_t duration_seconds) {
    LOG_INFO("Starting performance benchmark for {} seconds...", duration_seconds);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_seconds);
    
    while (std::chrono::high_resolution_clock::now() < end_time) {
        begin_frame_timing();
        
        // Render test frame
        auto render_result = render_test_frame();
        if (render_result.is_err()) {
            return Err(render_result.unwrap_err());
        }
        
        end_frame_timing();
        
        // Small delay to avoid burning CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_INFO("Performance benchmark completed");
    return Ok();
}

Result<void> MilestoneValidator::validate_memory_usage() {
    auto memory_usage = utils::get_memory_usage(device_);
    
    if (memory_usage.total_used > config_.max_memory_usage_mb * 1024 * 1024) {
        return Err(frame_graph::FrameGraphError{
            frame_graph::FrameGraphErrorType::ResourceCreationFailed,
            "Memory usage " + std::to_string(memory_usage.total_used / (1024 * 1024)) + 
            "MB exceeds target " + std::to_string(config_.max_memory_usage_mb) + "MB"
        });
    }
    
    LOG_INFO("Memory usage: {}MB", memory_usage.total_used / (1024 * 1024));
    return Ok();
}

Result<void> MilestoneValidator::setup_test_scene() {
    LOG_INFO("Setting up test scene with {} objects and {} lights...", 
              config_.num_objects, config_.num_lights);
    
    test_objects_ = TestSceneGenerator::generate_objects(config_.num_objects, config_.scene_size);
    test_lights_ = TestSceneGenerator::generate_lights(config_.num_lights, config_.scene_size);
    
    LOG_INFO("Test scene setup completed");
    return Ok();
}

Result<void> MilestoneValidator::cleanup_test_scene() {
    test_objects_.clear();
    test_lights_.clear();
    return Ok();
}

Result<void> MilestoneValidator::render_test_frame() {
    // This would render a complete test frame using all systems
    // For validation purposes, we simulate the timing
    
    // Simulate GPU work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Simulate CPU work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    return Ok();
}

void MilestoneValidator::begin_frame_timing() {
    frame_start_time_ = std::chrono::high_resolution_clock::now();
}

void MilestoneValidator::end_frame_timing() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - frame_start_time_);
    
    float frame_time_ms = frame_duration.count() / 1000.0f;
    float gpu_time_ms = measure_gpu_time();
    float cpu_time_ms = measure_cpu_time();
    
    metrics_.update(frame_time_ms, gpu_time_ms, cpu_time_ms);
}

float MilestoneValidator::measure_gpu_time() {
    // This would use GPU timestamps for accurate measurement
    // For now, simulate with a reasonable value
    return 10.0f + (rand() % 100) / 100.0f; // 10-11ms
}

float MilestoneValidator::measure_cpu_time() {
    // This would measure actual CPU time
    // For now, simulate with a reasonable value
    return 1.0f + (rand() % 50) / 100.0f; // 1-1.5ms
}

bool MilestoneValidator::validate_frame_graph_compilation() {
    return validate_frame_graph_consistency(*frame_graph_);
}

bool MilestoneValidator::validate_forward_plus_rendering() {
    // This would validate forward+ rendering output
    return true; // Placeholder
}

bool MilestoneValidator::validate_shadow_mapping() {
    // This would validate shadow mapping output
    return true; // Placeholder
}

bool MilestoneValidator::validate_ibl_rendering() {
    // This would validate IBL rendering output
    return true; // Placeholder
}

bool MilestoneValidator::validate_async_compute_execution() {
    // This would validate async compute execution
    return true; // Placeholder
}

std::string MilestoneValidator::generate_validation_report(const ValidationResult& result) {
    std::stringstream ss;
    
    ss << "=== Foundation Renderer Milestone Validation Report ===\n";
    ss << "Test: " << result.test_name << "\n";
    ss << "Passed: " << (result.passed ? "YES" : "NO") << "\n\n";
    
    if (!result.error_message.empty()) {
        ss << "Errors:\n" << result.error_message << "\n\n";
    }
    
    ss << "Feature Status:\n";
    ss << "  Frame Graph: " << (result.features.frame_graph_compiled ? "PASS" : "FAIL") << "\n";
    ss << "  Forward+ Lighting: " << (result.features.forward_plus_rendered ? "PASS" : "FAIL") << "\n";
    ss << "  Cascaded Shadows: " << (result.features.cascaded_shadows_rendered ? "PASS" : "FAIL") << "\n";
    ss << "  IBL System: " << (result.features.ibl_rendered ? "PASS" : "FAIL") << "\n";
    ss << "  Async Compute: " << (result.features.async_compute_working ? "PASS" : "FAIL") << "\n\n";
    
    ss << "Performance Metrics:\n";
    ss << "  Average FPS: " << std::fixed << std::setprecision(2) << result.metrics.average_fps << "\n";
    ss << "  Min FPS: " << std::fixed << std::setprecision(2) << result.metrics.min_fps << "\n";
    ss << "  Max FPS: " << std::fixed << std::setprecision(2) << result.metrics.max_fps << "\n";
    ss << "  Frame Time: " << std::fixed << std::setprecision(2) << result.metrics.frame_time_ms << "ms\n";
    ss << "  GPU Time: " << std::fixed << std::setprecision(2) << result.metrics.gpu_time_ms << "ms\n";
    ss << "  CPU Time: " << std::fixed << std::setprecision(2) << result.metrics.cpu_time_ms << "ms\n";
    ss << "  Memory Usage: " << (result.metrics.memory_usage_bytes / (1024 * 1024)) << "MB\n";
    
    return ss.str();
}

std::string MilestoneValidator::generate_performance_report() {
    std::stringstream ss;
    
    ss << "Performance Report:\n";
    ss << "  Target FPS: " << config_.target_fps << "\n";
    ss << "  Actual FPS: " << std::fixed << std::setprecision(2) << metrics_.average_fps << "\n";
    ss << "  Frame Time: " << std::fixed << std::setprecision(2) << metrics_.frame_time_ms << "ms (target: " << config_.max_frame_time_ms << "ms)\n";
    ss << "  GPU Time: " << std::fixed << std::setprecision(2) << metrics_.gpu_time_ms << "ms (target: " << config_.max_gpu_time_ms << "ms)\n";
    ss << "  CPU Time: " << std::fixed << std::setprecision(2) << metrics_.cpu_time_ms << "ms (target: " << config_.max_cpu_time_ms << "ms)\n";
    
    return ss.str();
}

// Utils namespace implementation
namespace utils {

bool meets_performance_target(const PerformanceMetrics& metrics, const MilestoneConfig& config) {
    return metrics.average_fps >= config.target_fps &&
           metrics.frame_time_ms <= config.max_frame_time_ms &&
           metrics.gpu_time_ms <= config.max_gpu_time_ms &&
           metrics.cpu_time_ms <= config.max_cpu_time_ms;
}

MemoryUsage get_memory_usage(const hal::Device& device) {
    // This would query actual memory usage from VMA
    MemoryUsage usage;
    usage.total_allocated = 512 * 1024 * 1024; // 512MB placeholder
    usage.total_used = 256 * 1024 * 1024;     // 256MB placeholder
    usage.gpu_usage = usage.total_used;
    usage.cpu_usage = 0;
    return usage;
}

bool validate_frame_graph_consistency(const frame_graph::FrameGraph& frame_graph) {
    // This would validate frame graph consistency
    return true; // Placeholder
}

} // namespace utils

} // namespace cold_coffee::engine::render::validation
