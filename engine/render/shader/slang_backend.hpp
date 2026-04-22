#pragma once
// engine/render/shader/slang_backend.hpp — ADR-0074 Wave 17A.
//
// Optional Slang compilation path, gated behind GW_ENABLE_SLANG.
// The header exposes only POD types; <slang.h> is strictly confined to
// slang_backend.cpp.

#include <cstdint>
#include <string>
#include <vector>

namespace gw::render::shader {

enum class SlangStage : std::uint8_t {
    Vertex = 0,
    Fragment,
    Compute,
    Geometry,
    TessControl,
    TessEval,
    Mesh,
    Task,
};

enum class SlangTarget : std::uint8_t {
    SpirV_1_6 = 0,
    SpirV_1_5 = 1,
};

struct SlangCompileRequest {
    std::string               source;          // raw HLSL or Slang source
    std::string               entry_point;     // "main" by default
    SlangStage                stage{SlangStage::Vertex};
    SlangTarget               target{SlangTarget::SpirV_1_6};
    std::vector<std::string>  defines;         // "KEY=VALUE" strings
    std::vector<std::string>  include_dirs;    // absolute paths
};

struct SlangCompileResult {
    bool                       success{false};
    std::vector<std::uint32_t> spirv_words;
    std::string                error_log;
    std::string                compiler_version;  // populated on success
};

// Runtime probe — true when the binary was built with GW_ENABLE_SLANG=1.
[[nodiscard]] bool slang_available() noexcept;

// Compile. Under GW_ENABLE_SLANG=0 the call immediately returns success=false
// with error_log="slang: disabled (GW_ENABLE_SLANG=0)"; under =1 it invokes
// the SDK. The contract (inputs/outputs) is identical.
[[nodiscard]] SlangCompileResult slang_compile(const SlangCompileRequest&);

// Round-trip helper used by studio-* CI: compile with Slang and DXC both,
// assert both produce valid SPIR-V with identical workgroup-size / entry.
// Returns an error string on mismatch (empty on match).
[[nodiscard]] std::string slang_dxc_roundtrip_check(const SlangCompileRequest& req,
                                                    const std::vector<std::uint32_t>& dxc_spirv);

} // namespace gw::render::shader
