#include <doctest/doctest.h>
#include "engine/render/frame_graph/frame_graph.hpp"
#include "engine/render/frame_graph/types.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <string>

using namespace gw::render::frame_graph;

TEST_CASE("Frame Graph - Simple Two Pass Chain") {
    auto fg = std::make_unique<FrameGraph>();
    // Create resources
    TextureDesc color_desc{};
    color_desc.width = 1920;
    color_desc.height = 1080;
    color_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_desc.name = "color_target";
    
    auto color_result = fg->declare_texture(color_desc);
    CHECK(color_result.has_value());
    ResourceHandle color = color_result.value();
    
    // Create first pass (writes to color)
    std::vector<ResourceHandle> pass1_writes = {color};
    PassDesc pass1("RenderPass");
    pass1.writes = pass1_writes;
    pass1.execute = [](CommandBuffer& /*cmd*/) { /* Mock implementation */ };
    
    auto pass1_result = fg->add_pass(std::move(pass1));
    CHECK(pass1_result.has_value());
    
    // Create second pass (reads from color)
    std::vector<ResourceHandle> pass2_reads = {color};
    PassDesc pass2("PostProcessPass");
    pass2.reads = pass2_reads;
    pass2.execute = [](CommandBuffer& /*cmd*/) { /* Mock implementation */ };
    
    auto pass2_result = fg->add_pass(std::move(pass2));
    CHECK(pass2_result.has_value());
    
    // Compile should succeed
    auto compile_result = fg->compile();
    CHECK(compile_result.has_value());
    
    // Check execution order (pass1 should come before pass2)
    const auto& order = fg->execution_order();
    CHECK(order.size() == 2);
    CHECK(order[0] == pass1_result.value());
    CHECK(order[1] == pass2_result.value());
}

TEST_CASE("Frame Graph - Diamond Dependency") {
    auto fg = std::make_unique<FrameGraph>();
    // Create resources
    TextureDesc input_desc{};
    input_desc.width = 1024;
    input_desc.height = 1024;
    input_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    input_desc.name = "input";
    
    TextureDesc temp_desc{};
    temp_desc.width = 1024;
    temp_desc.height = 1024;
    temp_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    temp_desc.name = "temp";
    
    TextureDesc output_desc{};
    output_desc.width = 1024;
    output_desc.height = 1024;
    output_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    output_desc.name = "output";
    
    auto input_res = fg->declare_texture(input_desc);
    auto temp_res = fg->declare_texture(temp_desc);
    auto output_res = fg->declare_texture(output_desc);
    
    CHECK(input_res.has_value());
    CHECK(temp_res.has_value());
    CHECK(output_res.has_value());
    
    // Pass 1: input -> temp
    std::vector<ResourceHandle> p1_reads = {input_res.value()};
    std::vector<ResourceHandle> p1_writes = {temp_res.value()};
    PassDesc pass1("Pass1");
    pass1.reads = p1_reads;
    pass1.writes = p1_writes;
    pass1.execute = [](CommandBuffer& /*cmd*/) {};
    
    // Pass 2: input -> temp (parallel with Pass1)
    std::vector<ResourceHandle> p2_reads = {input_res.value()};
    std::vector<ResourceHandle> p2_writes = {temp_res.value()};
    PassDesc pass2("Pass2");
    pass2.reads = p2_reads;
    pass2.writes = p2_writes;
    pass2.execute = [](CommandBuffer& /*cmd*/) {};
    
    // Pass 3: temp -> output
    std::vector<ResourceHandle> p3_reads = {temp_res.value()};
    std::vector<ResourceHandle> p3_writes = {output_res.value()};
    PassDesc pass3("Pass3");
    pass3.reads = p3_reads;
    pass3.writes = p3_writes;
    pass3.execute = [](CommandBuffer& /*cmd*/) {};
    
    auto p1_handle = fg->add_pass(std::move(pass1));
    auto p2_handle = fg->add_pass(std::move(pass2));
    auto p3_handle = fg->add_pass(std::move(pass3));
    
    CHECK(p1_handle.has_value());
    CHECK(p2_handle.has_value());
    CHECK(p3_handle.has_value());
    
    // Compile should succeed
    auto compile_result = fg->compile();
    CHECK(compile_result.has_value());
    
    // Pass3 should come after Pass1 and Pass2
    const auto& order = fg->execution_order();
    CHECK(order.size() == 3);
    
    // Pass1 and Pass2 should come before Pass3
    auto p3_pos = std::find(order.begin(), order.end(), p3_handle.value());
    auto p1_pos = std::find(order.begin(), order.end(), p1_handle.value());
    auto p2_pos = std::find(order.begin(), order.end(), p2_handle.value());
    
    CHECK(p1_pos < p3_pos);
    CHECK(p2_pos < p3_pos);
}

TEST_CASE("Frame Graph - Cycle Detection") {
    auto fg = std::make_unique<FrameGraph>();
    // Create resources
    TextureDesc desc{};
    desc.width = 512;
    desc.height = 512;
    desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    desc.name = "resource";
    
    auto res1 = fg->declare_texture(desc);
    auto res2 = fg->declare_texture(desc);
    
    CHECK(res1.has_value());
    CHECK(res2.has_value());
    
    // Pass 1: reads res2, writes res1
    std::vector<ResourceHandle> p1_reads = {res2.value()};
    std::vector<ResourceHandle> p1_writes = {res1.value()};
    PassDesc pass1("Pass1");
    pass1.reads = p1_reads;
    pass1.writes = p1_writes;
    pass1.execute = [](CommandBuffer& /*cmd*/) {};
    
    // Pass 2: reads res1, writes res2 (creates cycle)
    std::vector<ResourceHandle> p2_reads = {res1.value()};
    std::vector<ResourceHandle> p2_writes = {res2.value()};
    PassDesc pass2("Pass2");
    pass2.reads = p2_reads;
    pass2.writes = p2_writes;
    pass2.execute = [](CommandBuffer& /*cmd*/) {};
    
    CHECK(fg->add_pass(std::move(pass1)).has_value());
    CHECK(fg->add_pass(std::move(pass2)).has_value());
    
    // Compile should fail with cycle error
    auto compile_result = fg->compile();
    CHECK(!compile_result.has_value());
    CHECK(compile_result.error().type == FrameGraphErrorType::CycleDetected);
}

TEST_CASE("Frame Graph - Undeclared Resource Error") {
    auto fg = std::make_unique<FrameGraph>();
    // Try to create a pass that references an undeclared resource
    std::vector<ResourceHandle> reads = {999};  // Invalid handle
    PassDesc pass("InvalidPass");
    pass.reads = reads;
    pass.execute = [](CommandBuffer& /*cmd*/) {};
    
    auto result = fg->add_pass(std::move(pass));
    CHECK(!result.has_value());
    CHECK(result.error().type == FrameGraphErrorType::UndeclaredResource);
}

TEST_CASE("Frame Graph - Resource Declaration") {
    auto fg = std::make_unique<FrameGraph>();
    // Test texture declaration
    TextureDesc tex_desc{};
    tex_desc.width = 256;
    tex_desc.height = 256;
    tex_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    tex_desc.name = "test_texture";
    
    auto tex_result = fg->declare_texture(tex_desc);
    CHECK(tex_result.has_value());
    CHECK(tex_result.value() == 0);
    
    // Test buffer declaration
    BufferDesc buf_desc{};
    buf_desc.size = 1024 * 1024;  // 1MB
    buf_desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buf_desc.name = "test_buffer";
    
    auto buf_result = fg->declare_buffer(buf_desc);
    CHECK(buf_result.has_value());
    CHECK(buf_result.value() == 1);  // Second resource
}

TEST_CASE("Multi-queue scheduler - empty execution order schedules without error") {
    MultiQueueScheduler sched(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
    std::vector<PassData> passes;
    std::vector<PassHandle> order;
    std::vector<PassBarriers> barriers;
    auto r = sched.schedule(passes, order, barriers);
    CHECK(r.has_value());
    CHECK(r->submissions.empty());
}

TEST_CASE("Multi-queue scheduler - non-empty order returns P20-FRAMEGRAPH-CMDBUF error") {
    MultiQueueScheduler sched(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
    PassDesc pd("solo");
    pd.execute = [](CommandBuffer&) {};
    std::vector<PassData> passes;
    passes.emplace_back(PassData(std::move(pd)));
    std::vector<PassHandle> order = {0};
    std::vector<PassBarriers> barriers(order.size());
    auto r = sched.schedule(passes, order, barriers);
    CHECK(!r.has_value());
    CHECK(r.error().type == FrameGraphErrorType::CompilationFailed);
    CHECK(r.error().message.find("P20-FRAMEGRAPH-CMDBUF") != std::string::npos);
}

TEST_CASE("Frame Graph - Validation") {
    auto fg = std::make_unique<FrameGraph>();
    // Create valid resources and passes
    TextureDesc desc{};
    desc.width = 128;
    desc.height = 128;
    desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    desc.name = "valid_resource";
    
    auto res = fg->declare_texture(desc);
    CHECK(res.has_value());
    
    std::vector<ResourceHandle> writes = {res.value()};
    PassDesc pass("ValidPass");
    pass.writes = writes;
    pass.execute = [](CommandBuffer& /*cmd*/) {};
    
    CHECK(fg->add_pass(std::move(pass)).has_value());
    
    // Validation should succeed
    auto validation_result = fg->validate();
    CHECK(validation_result.has_value());
}
