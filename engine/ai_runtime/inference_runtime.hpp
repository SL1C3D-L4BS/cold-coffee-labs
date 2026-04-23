#pragma once

#include "engine/ai_runtime/ai_runtime_types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace gw::ai_runtime {

struct InferenceError {
    enum class Code : std::uint8_t {
        Ok                   = 0,
        ModelNotFound        = 1,
        BackendUnsupported   = 2,
        TensorShapeMismatch  = 3,
        DeterminismViolation = 4,
        BudgetExceeded       = 5,
    };
    Code             code{Code::Ok};
    std::string_view message{};
};

class InferenceRuntime {
public:
    [[nodiscard]] static InferenceRuntime make_cpu() noexcept;
    [[nodiscard]] static InferenceRuntime make_vulkan_presentation_only() noexcept;

    /// Synchronous forward pass.
    ///
    /// Authoritative callers (Director) provide a CPU backend runtime; the
    /// function is a pure function of (`input`, model weights) — see
    /// docs/05 §14 for the ten-rule deterministic-ML coding standard.
    [[nodiscard]] InferenceError forward(ModelId                    model,
                                         std::span<const std::byte> input,
                                         std::span<std::byte>       output,
                                         std::uint64_t*             out_elapsed_ns = nullptr) noexcept;

    [[nodiscard]] Backend backend() const noexcept { return backend_; }

private:
    explicit InferenceRuntime(Backend backend) noexcept : backend_{backend} {}
    Backend backend_;
};

} // namespace gw::ai_runtime
