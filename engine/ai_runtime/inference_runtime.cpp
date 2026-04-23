// engine/ai_runtime/inference_runtime.cpp
//
// Phase 26 scaffold — binding to ggml / llama.cpp C++ headers lands in Phase 26
// week-02. Until the dependency is wired through CPM (ADR-0095 follow-up), the
// implementation returns `BackendUnsupported` so downstream consumers can build
// against the public header without dragging in a half-integrated runtime.

#include "engine/ai_runtime/inference_runtime.hpp"

namespace gw::ai_runtime {

InferenceRuntime InferenceRuntime::make_cpu() noexcept {
    return InferenceRuntime{Backend::CpuGgml};
}

InferenceRuntime InferenceRuntime::make_vulkan_presentation_only() noexcept {
    return InferenceRuntime{Backend::VulkanGgml};
}

InferenceError InferenceRuntime::forward(ModelId /*model*/,
                                         std::span<const std::byte> /*input*/,
                                         std::span<std::byte> /*output*/,
                                         std::uint64_t*             out_elapsed_ns) noexcept {
    if (out_elapsed_ns != nullptr) {
        *out_elapsed_ns = 0;
    }
    return InferenceError{
        InferenceError::Code::BackendUnsupported,
        "ai_runtime scaffold: ggml backend wiring pending (Phase 26 week-02)",
    };
}

} // namespace gw::ai_runtime
