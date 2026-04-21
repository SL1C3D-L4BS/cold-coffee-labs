// tests/smoke/bld_abi_v1/bld_abi_smoke_test.cpp
// Phase 7 week 042 — BLD C-ABI v1 smoke site (ADR-0007 §2.7).
//
// The ADR's prose smoke builds a separate dylib and loads it via
// DynamicLibrary. That costs a small harness of per-platform linker dancing
// (import libs on Windows, -rdynamic on Linux) that isn't pulling its weight
// at week 042 — the point of the freeze is to lock the function-pointer
// call chain, not the dynamic loader plumbing. This test binds against the
// Surface-P exports directly and drives the same sequence a real plugin
// would run. Upgrading to a true cross-dylib harness is a Phase-9 task and
// reuses every line of body below.
//
// Sequence under test (ADR-0007 §2.7 numbered list):
//   1. Editor (= this test) allocates a BldRegistrar.
//   2. Plugin's bld_abi_version() returns BLD_ABI_VERSION; editor checks it.
//   3. Plugin's bld_register() calls bld_registrar_register_tool(...).
//   4. Editor calls gw_editor_run_tool("test.hello").
//   5. Invoked tool calls gw_editor_log_error("bld_abi v1 round-trip OK").
//   6. Editor's log sink captured the message.

#include <doctest/doctest.h>

#include "editor/bld_api/editor_bld_api.hpp"
#include "bld/include/bld_register.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace {

// Test-owned log sink. Installed into EditorGlobals for the duration of the
// test so gw_editor_log{,_error} diverts here instead of stderr. Reset to
// nullptr on the way out so unrelated tests don't see leaked state.
std::string g_last_log_message;
std::uint32_t g_last_log_level = UINT32_MAX;

void test_log_sink(std::uint32_t level, const char* message) {
    g_last_log_level   = level;
    g_last_log_message = message ? message : "";
}

// ---------------------------------------------------------------------------
// Fake plugin ("smoke dylib") entry points. Same signatures the real BLD
// dylib will export; here they're local so we can drive them from the test.
// ---------------------------------------------------------------------------

bool g_say_hi_called = false;
void say_hi_fn(void* user_data) {
    g_say_hi_called = true;
    // The ADR-prescribed round-trip line — tests downstream of the log sink
    // assert on this literal, so BLD-side ports must keep it intact.
    gw_editor_log_error("bld_abi v1 round-trip OK");
    (void)user_data;
}

std::uint32_t smoke_plugin_bld_abi_version() {
    return BLD_ABI_VERSION;
}

bool smoke_plugin_bld_register(BldEditorHandle* /*host*/, BldRegistrar* reg) {
    const std::uint32_t tool_id = bld_registrar_register_tool(
        reg,
        "test.hello",
        "Hello",
        "Say hi",
        /*icon*/ nullptr,
        &say_hi_fn,
        /*user_data*/ nullptr);
    return tool_id != 0u;
}

}  // namespace

TEST_CASE("bld_abi_v1 — version + registrar + tool round-trip") {
    using namespace gw::editor::bld_api;

    // Install our log sink; save whatever was there so we don't clobber it.
    const LogSinkFn prev_sink = g_globals.log_sink;
    g_globals.log_sink        = &test_log_sink;

    g_last_log_message.clear();
    g_last_log_level = UINT32_MAX;
    g_say_hi_called  = false;

    // --- Step 1: editor allocates a registrar. -------------------------
    BldRegistrar* reg = acquire_registrar();
    REQUIRE(reg != nullptr);

    // --- Step 2: plugin reports ABI version; editor checks compat. -----
    const std::uint32_t plugin_ver = smoke_plugin_bld_abi_version();
    CHECK(plugin_ver == BLD_ABI_VERSION);

    // --- Step 3: plugin's bld_register() registers a tool. -------------
    REQUIRE(smoke_plugin_bld_register(/*host=*/nullptr, reg));

    // --- Step 4: editor invokes the tool by id. ------------------------
    CHECK(gw_editor_run_tool("test.hello"));
    CHECK(g_say_hi_called);

    // --- Step 5 + 6: log sink received the advertised round-trip line. -
    CHECK(g_last_log_level == 2u);  // error
    CHECK(g_last_log_message == "bld_abi v1 round-trip OK");

    // Unknown tool ids are rejected cleanly.
    CHECK_FALSE(gw_editor_run_tool("nope.missing"));

    release_registrar(reg);
    // A freshly acquired registrar must succeed now that we've released.
    BldRegistrar* reg2 = acquire_registrar();
    REQUIRE(reg2 != nullptr);
    release_registrar(reg2);

    g_globals.log_sink = prev_sink;
}

TEST_CASE("bld_abi_v1 — duplicate tool id returns 0") {
    using namespace gw::editor::bld_api;
    BldRegistrar* reg = acquire_registrar();
    REQUIRE(reg != nullptr);

    auto noop = +[](void*) {};
    const std::uint32_t first = bld_registrar_register_tool(
        reg, "dup.id", "First", "", nullptr, noop, nullptr);
    const std::uint32_t second = bld_registrar_register_tool(
        reg, "dup.id", "Second", "", nullptr, noop, nullptr);

    CHECK(first != 0u);
    CHECK(second == 0u);

    release_registrar(reg);
}

TEST_CASE("bld_abi_v1 — widget registration returns monotonic ids") {
    using namespace gw::editor::bld_api;
    BldRegistrar* reg = acquire_registrar();
    REQUIRE(reg != nullptr);

    auto draw = +[](void*, void*) {};
    const std::uint32_t a = bld_registrar_register_widget(reg, 0xAAu, draw, nullptr);
    const std::uint32_t b = bld_registrar_register_widget(reg, 0xBBu, draw, nullptr);

    CHECK(a != 0u);
    CHECK(b != 0u);
    CHECK(b > a);
    CHECK_FALSE(bld_registrar_register_widget(reg, 0xCCu, nullptr, nullptr));

    release_registrar(reg);
}

TEST_CASE("bld_abi_v1 — log level clamps to error") {
    using namespace gw::editor::bld_api;
    const LogSinkFn prev_sink = g_globals.log_sink;
    g_globals.log_sink = &test_log_sink;
    g_last_log_level   = UINT32_MAX;

    gw_editor_log(99u, "clamp me");
    CHECK(g_last_log_level == 2u);
    CHECK(g_last_log_message == "clamp me");

    g_globals.log_sink = prev_sink;
}
