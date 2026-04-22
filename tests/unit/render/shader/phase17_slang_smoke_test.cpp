// tests/unit/render/shader/phase17_slang_smoke_test.cpp — ADR-0074 Wave 17A.

#include <doctest/doctest.h>

#include "engine/render/shader/slang_backend.hpp"
#include "engine/render/shader/reflect.hpp"

using namespace gw::render::shader;

TEST_CASE("phase17.slang: availability probe matches build flag") {
#if defined(GW_ENABLE_SLANG) && GW_ENABLE_SLANG
    CHECK(slang_available());
#else
    CHECK_FALSE(slang_available());
#endif
}

TEST_CASE("phase17.slang: disabled backend returns disabled error log") {
    if (slang_available()) return;
    SlangCompileRequest req{};
    req.source      = "void main(){}";
    req.entry_point = "main";
    const auto r = slang_compile(req);
    CHECK_FALSE(r.success);
    CHECK(r.error_log.find("disabled") != std::string::npos);
}

TEST_CASE("phase17.slang: enabled backend emits SPIR-V header") {
    if (!slang_available()) return;
    SlangCompileRequest req{};
    req.source = "void main(){}";
    const auto r = slang_compile(req);
    CHECK(r.success);
    CHECK(r.spirv_words.size() >= 5);
    CHECK(looks_like_spirv(r.spirv_words.data(), r.spirv_words.size()));
}

TEST_CASE("phase17.slang: roundtrip guard reports disabled when slang off") {
    if (slang_available()) return;
    SlangCompileRequest req{};
    const auto err = slang_dxc_roundtrip_check(req, {});
    CHECK_FALSE(err.empty());
}

TEST_CASE("phase17.slang: request struct carries defines and include dirs") {
    SlangCompileRequest req{};
    req.defines.push_back("GW_HAS_CLEARCOAT=1");
    req.include_dirs.push_back("/shaders/include");
    CHECK(req.defines.size() == 1);
    CHECK(req.include_dirs.size() == 1);
}

TEST_CASE("phase17.slang: stage enum default is Vertex") {
    SlangCompileRequest req{};
    CHECK(req.stage == SlangStage::Vertex);
    CHECK(req.target == SlangTarget::SpirV_1_6);
}
