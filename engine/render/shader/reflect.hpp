#pragma once
// engine/render/shader/reflect.hpp — ADR-0074 Wave 17A.
//
// Opaque SPIR-V reflection wrapper. The actual SPIRV-Reflect SDK lives
// behind GW_ENABLE_SPIRV_REFLECT in reflect.cpp; the public surface is
// POD types describing descriptor set layouts, push constants, and
// vertex inputs — no SDK types leak into headers.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gw::render::shader {

enum class DescriptorType : std::uint8_t {
    Unknown = 0,
    UniformBuffer,
    StorageBuffer,
    SampledImage,
    StorageImage,
    CombinedImageSampler,
    Sampler,
    InputAttachment,
    UniformBufferDynamic,
    StorageBufferDynamic,
};

enum class ShaderStageBit : std::uint32_t {
    None        = 0,
    Vertex      = 1u << 0,
    Fragment    = 1u << 1,
    Compute     = 1u << 2,
    Geometry    = 1u << 3,
    TessControl = 1u << 4,
    TessEval    = 1u << 5,
    Mesh        = 1u << 6,
    Task        = 1u << 7,
};

struct DescriptorBinding {
    std::string    name;
    std::uint32_t  set{0};
    std::uint32_t  binding{0};
    DescriptorType type{DescriptorType::Unknown};
    std::uint32_t  count{1};
    std::uint32_t  stage_mask{0};
};

struct PushConstantRange {
    std::uint32_t offset_bytes{0};
    std::uint32_t size_bytes{0};
    std::uint32_t stage_mask{0};
};

struct VertexInput {
    std::string   semantic;
    std::uint32_t location{0};
    std::uint32_t component_count{0};   // vec4 = 4
    std::uint32_t component_size{0};    // float = 4
};

struct WorkgroupSize {
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t z{0};
    [[nodiscard]] std::uint32_t total() const noexcept { return x * y * z; }
};

struct ReflectionReport {
    std::uint32_t                  spirv_words{0};
    std::uint32_t                  stage_mask{0};
    std::string                    entry_point;
    std::vector<DescriptorBinding> bindings;
    std::vector<PushConstantRange> push_constants;
    std::vector<VertexInput>       vertex_inputs;
    std::optional<WorkgroupSize>   workgroup_size;
    std::string                    error;    // empty if ok

    [[nodiscard]] bool ok() const noexcept { return error.empty(); }
};

// Run SPIRV-Reflect over a SPIR-V word stream. Under GW_ENABLE_SPIRV_REFLECT=0
// a deterministic null-path implementation parses the SPIR-V header only
// (magic + version + entry-point) — sufficient for dev-* CI to exercise
// the plumbing but not the full SDK.
[[nodiscard]] ReflectionReport reflect_spirv(const std::uint32_t* words,
                                              std::size_t word_count);

// Convenience wrapper over a byte span.
[[nodiscard]] ReflectionReport reflect_spirv_bytes(const std::byte* bytes,
                                                    std::size_t byte_count);

// True iff the first 4 words look like a valid SPIR-V module header
// (magic = 0x07230203, bounded version).
[[nodiscard]] bool looks_like_spirv(const std::uint32_t* words,
                                     std::size_t word_count) noexcept;

} // namespace gw::render::shader
