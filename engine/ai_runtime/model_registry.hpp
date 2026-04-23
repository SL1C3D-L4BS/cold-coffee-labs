#pragma once

#include "engine/ai_runtime/ai_runtime_types.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gw::ai_runtime {

struct ModelRecord {
    ModelId      id{};
    ModelKind    kind{ModelKind::DirectorPolicy};
    Backend      backend{Backend::CpuGgml};
    ModelHash    hash{};
    std::uint64_t params_count = 0;
    float        budget_ms     = 0.f;
    bool         presentation_only = false;
};

class ModelRegistry {
public:
    [[nodiscard]] static ModelRegistry& instance() noexcept;

    /// Register a pinned weight file. The registry computes BLAKE3 over the
    /// weight blob and verifies the Ed25519 signature before returning a valid
    /// `ModelId`. Phase 26 scaffold returns an invalid id.
    [[nodiscard]] ModelId register_pinned(std::string_view weight_path,
                                          ModelKind        kind,
                                          Backend          backend,
                                          float            budget_ms,
                                          bool             presentation_only) noexcept;

    [[nodiscard]] const ModelRecord* find(ModelId id) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    ModelRegistry() noexcept = default;

    static constexpr std::size_t MAX_MODELS = 32;
    ModelRecord records_[MAX_MODELS]{};
    std::size_t count_ = 0;
};

} // namespace gw::ai_runtime
