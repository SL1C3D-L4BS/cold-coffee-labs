// tests/unit/editor/reflect_macro_test.cpp
// Phase 7 gate: GW_REFLECT correctness — spec §17.
//
// <ostream> MUST be first: MSVC STL 14.44 + clang-cl defines
// operator<<(ostream, string_view) inline in __msvc_string_view.hpp and needs
// the COMPLETE basic_ostream.  Doctest only forward-declares it in
// non-implement TUs, so we force the full definition before doctest sees it.
#include <ostream>
#include <doctest/doctest.h>
#include "editor/reflect/reflect.hpp"

#include <cstddef>
#include <cstring>

using namespace gw::editor::reflect;
using namespace gw::core;

// ---------------------------------------------------------------------------
// Test structs.
// ---------------------------------------------------------------------------

struct EightFieldComp {
    float a = 1.f;
    float b = 2.f;
    int32_t c = 3;
    bool d = true;
    float e = 5.f;
    float f = 6.f;
    float g = 7.f;
    int32_t h = 8;

    GW_REFLECT(EightFieldComp, a, b, c, d, e, f, g, h)
};

struct SingleFieldComp {
    float x = 42.f;
    GW_REFLECT(SingleFieldComp, x)
};

// ---------------------------------------------------------------------------
TEST_CASE("GW_REFLECT — 8-field struct: correct count") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    CHECK(ti.fields.size() == 8);
}

TEST_CASE("GW_REFLECT — type_name is correct") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    CHECK(strcmp(ti.type_name, "EightFieldComp") == 0);
}

TEST_CASE("GW_REFLECT — type_id is FNV1a of type name") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    CHECK(ti.type_id == fnv1a("EightFieldComp"));
}

TEST_CASE("GW_REFLECT — field names correct") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    const char* expected[] = {"a","b","c","d","e","f","g","h"};
    for (size_t i = 0; i < 8; ++i)
        CHECK(strcmp(ti.fields[i].name, expected[i]) == 0);
}

TEST_CASE("GW_REFLECT — field offsets match offsetof") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    CHECK(ti.fields[0].offset == offsetof(EightFieldComp, a));
    CHECK(ti.fields[1].offset == offsetof(EightFieldComp, b));
    CHECK(ti.fields[2].offset == offsetof(EightFieldComp, c));
    CHECK(ti.fields[3].offset == offsetof(EightFieldComp, d));
    CHECK(ti.fields[4].offset == offsetof(EightFieldComp, e));
    CHECK(ti.fields[5].offset == offsetof(EightFieldComp, f));
    CHECK(ti.fields[6].offset == offsetof(EightFieldComp, g));
    CHECK(ti.fields[7].offset == offsetof(EightFieldComp, h));
}

TEST_CASE("GW_REFLECT — field sizes correct") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    CHECK(ti.fields[0].size == sizeof(float));
    CHECK(ti.fields[2].size == sizeof(int32_t));
    CHECK(ti.fields[3].size == sizeof(bool));
}

TEST_CASE("GW_REFLECT — field type_ids stable (FNV1a, not typeid hash)") {
    const TypeInfo& ti = describe(static_cast<EightFieldComp*>(nullptr));
    // All float fields share the same type_id.
    CHECK(ti.fields[0].type_id == ti.fields[1].type_id);
    // float vs int32_t differ.
    CHECK(ti.fields[0].type_id != ti.fields[2].type_id);
}

TEST_CASE("GW_REFLECT — field pointer read-back correct") {
    EightFieldComp c;
    c.a = 99.f;
    const TypeInfo& ti = describe(&c);
    const float* ptr = reinterpret_cast<const float*>(
        reinterpret_cast<const uint8_t*>(&c) + ti.fields[0].offset);
    CHECK(*ptr == doctest::Approx(99.f));
}

TEST_CASE("GW_REFLECT — single-field struct works") {
    const TypeInfo& ti = describe(static_cast<SingleFieldComp*>(nullptr));
    CHECK(ti.fields.size() == 1);
    CHECK(strcmp(ti.fields[0].name, "x") == 0);
}
