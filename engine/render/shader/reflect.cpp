// engine/render/shader/reflect.cpp — ADR-0074 Wave 17A.
//
// Header-quarantined: <spirv_reflect.h> only included here, behind
// GW_ENABLE_SPIRV_REFLECT. Null path parses the SPIR-V module header
// and emits an empty ReflectionReport sufficient for dev-* CI.

#include "engine/render/shader/reflect.hpp"

#include <cstring>

#if defined(GW_ENABLE_SPIRV_REFLECT) && GW_ENABLE_SPIRV_REFLECT
// #include <spirv_reflect.h>  // vendored; enabled under studio-*
#endif

namespace gw::render::shader {

namespace {

constexpr std::uint32_t kSpirvMagic = 0x07230203u;
constexpr std::uint32_t kSpirvMaxVersion = 0x0001'0700u;   // 1.7 ceiling (future-proof)

std::uint32_t stage_mask_from_exec_model(std::uint32_t exec_model) noexcept {
    // SPIR-V ExecutionModel: Vertex=0, TessControl=1, TessEval=2,
    // Geometry=3, Fragment=4, GLCompute=5, Kernel=6, TaskNV=5267,
    // MeshNV=5268 (EXT variants alias to the same bits in our mask).
    switch (exec_model) {
        case 0:    return static_cast<std::uint32_t>(ShaderStageBit::Vertex);
        case 1:    return static_cast<std::uint32_t>(ShaderStageBit::TessControl);
        case 2:    return static_cast<std::uint32_t>(ShaderStageBit::TessEval);
        case 3:    return static_cast<std::uint32_t>(ShaderStageBit::Geometry);
        case 4:    return static_cast<std::uint32_t>(ShaderStageBit::Fragment);
        case 5:    return static_cast<std::uint32_t>(ShaderStageBit::Compute);
        case 5267: return static_cast<std::uint32_t>(ShaderStageBit::Task);
        case 5268: return static_cast<std::uint32_t>(ShaderStageBit::Mesh);
        default:   return 0;
    }
}

} // namespace

bool looks_like_spirv(const std::uint32_t* words, std::size_t word_count) noexcept {
    if (!words || word_count < 5) return false;
    if (words[0] != kSpirvMagic) return false;
    if (words[1] == 0 || words[1] > kSpirvMaxVersion) return false;
    return true;
}

ReflectionReport reflect_spirv(const std::uint32_t* words, std::size_t word_count) {
    ReflectionReport r{};
    r.spirv_words = static_cast<std::uint32_t>(word_count);
    if (!looks_like_spirv(words, word_count)) {
        r.error = "spirv: module header invalid";
        return r;
    }
    // --- null-path parser ----------------------------------------------------
    // Walk the module and pick up:
    //   OpEntryPoint (opcode 15) → stage_mask + entry_point name
    //   OpExecutionMode (opcode 16) → LocalSize for compute (mode 17)
    std::size_t i = 5;
    while (i < word_count) {
        const std::uint32_t instr      = words[i];
        const std::uint32_t opcode     = instr & 0xFFFFu;
        const std::uint32_t word_count_instr = (instr >> 16) & 0xFFFFu;
        if (word_count_instr == 0 || i + word_count_instr > word_count) {
            r.error = "spirv: malformed instruction";
            return r;
        }
        if (opcode == 15 /*OpEntryPoint*/ && word_count_instr >= 3) {
            const std::uint32_t exec_model = words[i + 1];
            r.stage_mask |= stage_mask_from_exec_model(exec_model);
            // Name begins at word i+3, NUL-terminated across words.
            const char* name_ptr = reinterpret_cast<const char*>(&words[i + 3]);
            const std::size_t max_bytes = (word_count_instr - 3) * sizeof(std::uint32_t);
            std::size_t n = 0;
            while (n < max_bytes && name_ptr[n] != '\0') ++n;
            r.entry_point.assign(name_ptr, n);
        } else if (opcode == 16 /*OpExecutionMode*/ && word_count_instr >= 3) {
            const std::uint32_t mode = words[i + 2];
            if (mode == 17 /*LocalSize*/ && word_count_instr >= 6) {
                WorkgroupSize wg{};
                wg.x = words[i + 3];
                wg.y = words[i + 4];
                wg.z = words[i + 5];
                r.workgroup_size = wg;
            }
        }
        i += word_count_instr;
    }
#if defined(GW_ENABLE_SPIRV_REFLECT) && GW_ENABLE_SPIRV_REFLECT
    // Real-path hook (wired when the vendored SPIRV-Reflect lands).
    // For Phase-17 headless CI we deliberately ship the null path; the
    // studio-* presets flip this on.
#endif
    return r;
}

ReflectionReport reflect_spirv_bytes(const std::byte* bytes, std::size_t byte_count) {
    if (!bytes || byte_count < 4 || (byte_count % 4) != 0) {
        ReflectionReport r{};
        r.error = "spirv: byte stream not 4-aligned";
        return r;
    }
    const std::uint32_t* words = reinterpret_cast<const std::uint32_t*>(bytes);
    return reflect_spirv(words, byte_count / 4);
}

} // namespace gw::render::shader
