#pragma once

#include <cstdint>

namespace gw::ai_runtime {

enum class ModelKind : std::uint8_t {
    DirectorPolicy    = 0,
    MusicSymbolic     = 1,
    NeuralMaterial    = 2,
};

enum class Backend : std::uint8_t {
    CpuGgml    = 0,
    VulkanGgml = 1,
};

struct PresentationOnlyTag {};
inline constexpr PresentationOnlyTag presentation_only{};

struct ModelId {
    std::uint32_t value = 0;
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    friend constexpr bool operator==(ModelId, ModelId) noexcept = default;
};

struct ModelHash {
    std::uint8_t blake3[32]{};
};

} // namespace gw::ai_runtime
