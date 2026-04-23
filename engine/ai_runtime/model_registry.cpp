// engine/ai_runtime/model_registry.cpp — Phase 26 scaffold.

#include "engine/ai_runtime/model_registry.hpp"

namespace gw::ai_runtime {

ModelRegistry& ModelRegistry::instance() noexcept {
    static ModelRegistry inst{};
    return inst;
}

ModelId ModelRegistry::register_pinned(std::string_view /*weight_path*/,
                                       ModelKind         kind,
                                       Backend           backend,
                                       float             budget_ms,
                                       bool              presentation_only) noexcept {
    if (count_ >= MAX_MODELS) {
        return ModelId{};
    }
    ModelRecord& rec      = records_[count_];
    rec.id.value          = static_cast<std::uint32_t>(count_ + 1);
    rec.kind              = kind;
    rec.backend           = backend;
    rec.budget_ms         = budget_ms;
    rec.presentation_only = presentation_only;
    // BLAKE3 + Ed25519 verify lands in Phase 26 week-02 (ADR-0096).
    ++count_;
    return rec.id;
}

const ModelRecord* ModelRegistry::find(ModelId id) const noexcept {
    if (!id.valid() || id.value > count_) {
        return nullptr;
    }
    return &records_[id.value - 1];
}

} // namespace gw::ai_runtime
