// tests/perf/foundation_renderer_perf_test.cpp
//
// Foundation Renderer milestone CTest performance gate.
//
// Runs in two modes:
//   Headless (default, no GPU):
//     — Validates frame graph compilation, pass ordering, cycle detection,
//       and barrier correctness entirely in software.  Always passes in CI.
//   GPU mode (set env GW_GPU_TESTS=1 before ctest):
//     — Additionally instantiates ClusteredLighting, CascadedShadowMapper,
//       IBLSystem via a real hal::VulkanDevice and checks hard perf targets.
//
// Exit codes:
//   0  all applicable tests pass
//   1  one or more tests failed (prints diagnostics to stderr)
//
// docs/05 §Phase 5 exit criteria (week 032).

#include "engine/render/frame_graph/frame_graph.hpp"
#include "engine/render/frame_graph/types.hpp"
#include "engine/render/frame_graph/error.hpp"
#include "engine/render/hal/vulkan_device.hpp"
#include "engine/render/hal/vulkan_instance.hpp"
#include "engine/render/validation/milestone_validator.hpp"

#include <volk.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

using namespace gw::render::frame_graph;

// ---------------------------------------------------------------------------
// Simple test harness (no doctest dependency — this is the perf binary, not
// the unit-test binary)
// ---------------------------------------------------------------------------

namespace {

int g_failed = 0;

void check(bool cond, const char* label) {
    if (cond) {
        std::fprintf(stdout, "  PASS  %s\n", label);
    } else {
        std::fprintf(stderr, "  FAIL  %s\n", label);
        ++g_failed;
    }
}

// ---------------------------------------------------------------------------
// Frame-graph headless tests
// ---------------------------------------------------------------------------

void test_fg_simple_chain() {
    std::fprintf(stdout, "\n[perf-gate] Frame Graph: simple two-pass chain\n");
    FrameGraph fg;

    TextureDesc d;
    d.width  = 1920;
    d.height = 1080;
    d.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d.name   = "color";
    auto color = fg.declare_texture(d);
    check(color.has_value(), "declare color target");

    PassDesc p1("DrawPass");
    p1.writes.push_back(color.value());
    p1.execute = [](CommandBuffer&) {};
    auto h1 = fg.add_pass(std::move(p1));
    check(h1.has_value(), "add DrawPass");

    PassDesc p2("PostFX");
    p2.reads.push_back(color.value());
    p2.execute = [](CommandBuffer&) {};
    auto h2 = fg.add_pass(std::move(p2));
    check(h2.has_value(), "add PostFX");

    auto comp = fg.compile();
    check(comp.has_value(), "compile succeeds");

    const auto& order = fg.execution_order();
    check(order.size() == 2, "order has 2 passes");
    if (order.size() == 2) {
        auto pos1 = std::find(order.begin(), order.end(), h1.value());
        auto pos2 = std::find(order.begin(), order.end(), h2.value());
        check(pos1 < pos2, "DrawPass precedes PostFX");
    }
}

void test_fg_cycle_detection() {
    std::fprintf(stdout, "\n[perf-gate] Frame Graph: cycle detection\n");
    FrameGraph fg;

    TextureDesc d;
    d.width = d.height = 512;
    d.format = VK_FORMAT_R8G8B8A8_UNORM;
    d.name = "a";
    auto ra = fg.declare_texture(d);
    d.name = "b";
    auto rb = fg.declare_texture(d);
    check(ra.has_value() && rb.has_value(), "declare two resources");

    PassDesc pa("PA");
    pa.reads.push_back(rb.value());
    pa.writes.push_back(ra.value());
    pa.execute = [](CommandBuffer&) {};
    [[maybe_unused]] const auto pass_a = fg.add_pass(std::move(pa));

    PassDesc pb("PB");
    pb.reads.push_back(ra.value());
    pb.writes.push_back(rb.value());
    pb.execute = [](CommandBuffer&) {};
    [[maybe_unused]] const auto pass_b = fg.add_pass(std::move(pb));

    auto comp = fg.compile();
    check(!comp.has_value(), "compile fails on cycle");
    if (!comp.has_value()) {
        check(comp.error().type == FrameGraphErrorType::CycleDetected, "error is CycleDetected");
    }
}

void test_fg_undeclared_resource() {
    std::fprintf(stdout, "\n[perf-gate] Frame Graph: undeclared resource guard\n");
    FrameGraph fg;
    PassDesc p("Bad");
    p.reads.push_back(static_cast<ResourceHandle>(0xDEAD));
    p.execute = [](CommandBuffer&) {};
    auto r = fg.add_pass(std::move(p));
    check(!r.has_value(), "add_pass rejects bad handle");
    if (!r.has_value()) {
        check(r.error().type == FrameGraphErrorType::UndeclaredResource,
              "error is UndeclaredResource");
    }
}

void test_fg_mixed_queue_passes() {
    std::fprintf(stdout, "\n[perf-gate] Frame Graph: compute+graphics mixed queue\n");
    FrameGraph fg;

    BufferDesc bd;
    bd.size  = 1024 * 1024;
    bd.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bd.name  = "light_grid";
    auto buf = fg.declare_buffer(bd);
    check(buf.has_value(), "declare buffer");

    PassDesc compute("LightCull");
    compute.queue = QueueType::Compute;
    compute.writes.push_back(buf.value());
    compute.execute = [](CommandBuffer& cmd) { cmd.dispatch(16, 16, 1); };
    auto hc = fg.add_pass(std::move(compute));
    check(hc.has_value(), "add compute pass");

    PassDesc graphics("ForwardDraw");
    graphics.queue = QueueType::Graphics;
    graphics.reads.push_back(buf.value());
    graphics.execute = [](CommandBuffer&) {};
    auto hg = fg.add_pass(std::move(graphics));
    check(hg.has_value(), "add graphics pass");

    auto comp = fg.compile();
    check(comp.has_value(), "compile mixed-queue succeeds");
    if (comp.has_value()) {
        const auto& order = fg.execution_order();
        auto pc = std::find(order.begin(), order.end(), hc.value());
        auto pg = std::find(order.begin(), order.end(), hg.value());
        check(pc < pg, "LightCull precedes ForwardDraw");
    }
}

// ---------------------------------------------------------------------------
// Compile-time performance gate: ensure fg.compile() is fast for large graphs
// ---------------------------------------------------------------------------

void test_fg_compile_time() {
    std::fprintf(stdout, "\n[perf-gate] Frame Graph: compile time gate (Release<10ms / Debug<200ms, 256 passes)\n");

    constexpr int kPasses = 256;
    FrameGraph fg;

    // Create a linear chain resource → pass → resource → pass …
    TextureDesc d;
    d.width = d.height = 2;
    d.format = VK_FORMAT_R8_UNORM;

    d.name = "r0";
    auto prev = fg.declare_texture(d);

    for (int i = 0; i < kPasses; ++i) {
        d.name = "r" + std::to_string(i + 1);
        auto next = fg.declare_texture(d);
        PassDesc p("P" + std::to_string(i));
        if (prev.has_value()) p.reads.push_back(prev.value());
        if (next.has_value()) p.writes.push_back(next.value());
        p.execute = [](CommandBuffer&) {};
        [[maybe_unused]] const auto pass = fg.add_pass(std::move(p));
        prev = next;
    }

    const auto t0 = std::chrono::high_resolution_clock::now();
    auto comp = fg.compile();
    const auto t1 = std::chrono::high_resolution_clock::now();

    const float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
    std::fprintf(stdout, "    compile time: %.2f ms\n", ms);

    check(comp.has_value(), "large graph compiles");
    // Release gate: <10 ms.  Debug gate: <200 ms (unoptimised allocation overhead).
#ifdef NDEBUG
    check(ms < 10.0f,  "compile time < 10 ms (Release)");
#else
    check(ms < 200.0f, "compile time < 200 ms (Debug)");
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::fprintf(stdout, "=== Foundation Renderer Performance Gate ===\n");
    std::fprintf(stdout, "    gw::render::frame_graph headless tests\n");

    test_fg_simple_chain();
    test_fg_cycle_detection();
    test_fg_undeclared_resource();
    test_fg_mixed_queue_passes();
    test_fg_compile_time();

    // GPU mode (optional) — GW_GPU_TESTS=1. Full MilestoneValidator on a real
    // hal::VulkanDevice (incl. ~5 s performance benchmark; see milestone_validator).
    const char* gpu_env = std::getenv("GW_GPU_TESTS");
    if (gpu_env && std::string(gpu_env) == "1") {
        std::fprintf(stdout,
            "\n[perf-gate] GPU mode: MilestoneValidator (frame graph + forward+ + CSM + "
            "IBL + async + perf)\n");
        try {
            if (volkInitialize() != VK_SUCCESS) {
                check(false, "volkInitialize");
            } else {
                gw::render::hal::VulkanInstance vinst("gw_foundation_perf_gpu");
                VkInstance inst = vinst.native_handle();
                std::uint32_t n = 0;
                vkEnumeratePhysicalDevices(inst, &n, nullptr);
                if (n == 0) {
                    check(false, "vkEnumeratePhysicalDevices: zero GPUs");
                } else {
                    std::vector<VkPhysicalDevice> gpus(static_cast<std::size_t>(n));
                    vkEnumeratePhysicalDevices(inst, &n, gpus.data());
                    VkPhysicalDevice phys = gpus[0];
                    for (const auto g : gpus) {
                        VkPhysicalDeviceProperties p{};
                        vkGetPhysicalDeviceProperties(g, &p);
                        if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                            phys = g;
                            break;
                        }
                    }
                    gw::render::hal::VulkanDevice vdev(inst, phys);
                    check(vdev.native_handle() != VK_NULL_HANDLE,
                        "hal::VulkanDevice valid logical device (GPU smoke)");

                    gw::render::validation::MilestoneValidator   validator(vdev);
                    const gw::render::validation::MilestoneConfig full_cfg{};
                    const auto init = validator.initialize(full_cfg);
                    if (!init) {
                        check(false, "MilestoneValidator::initialize");
                    } else {
                        const auto milestone = validator.validate_foundation_renderer();
                        check(milestone.passed, "MilestoneValidator::validate_foundation_renderer");
                    }
                }
            }
        } catch (const std::exception& ex) {
            std::fprintf(stderr, "  GPU path exception: %s\n", ex.what());
            check(false, "GPU path exception");
        }
    }

    std::fprintf(stdout, "\n=== Results: %d failure(s) ===\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
