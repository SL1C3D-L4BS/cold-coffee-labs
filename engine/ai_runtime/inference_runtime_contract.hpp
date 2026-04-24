#pragma once
// engine/ai_runtime/inference_runtime_contract.hpp — Phase 26 living stack seam (plan Track J / phase26).

#include <cstdint>

namespace gw::ai_runtime {

/// Versioned contract between editor AI Director Sandbox and pinned runtime weights.
struct InferenceRuntimeContract {
    std::uint32_t schema_version = 1;
    std::uint32_t weight_manifest_crc32 = 0;
};

} // namespace gw::ai_runtime
