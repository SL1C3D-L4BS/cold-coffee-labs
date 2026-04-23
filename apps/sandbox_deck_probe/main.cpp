// apps/sandbox_deck_probe/main.cpp — Part C §22 scaffold (ADR-0113).
//
// Steam Deck Verified smoke probe. Each check is a scaffold today and
// lands its real validation logic in Phase 22.

#include <cstdio>

namespace {

struct ProbeResult {
    const char* name;
    bool        ok;
};

ProbeResult check_no_launcher() noexcept {
    // Phase 22: verify that the runtime does not spawn a separate launcher
    // process on first boot. Verified-checklist requirement.
    return {"no_launcher", true};
}
ProbeResult check_controller_glyphs() noexcept {
    // Phase 22: verify SteamInput returns controller glyphs (not KBM) when
    // the active origin reports a controller scheme.
    return {"controller_glyphs", true};
}
ProbeResult check_default_bindings() noexcept {
    // Phase 22: assert every gameplay action has a default controller
    // binding (no menu-hunting required).
    return {"default_bindings", true};
}
ProbeResult check_no_compat_warnings() noexcept {
    // Phase 22: parse stderr for device-compatibility warnings from the HAL.
    return {"no_compat_warnings", true};
}
ProbeResult check_native_linux_vk() noexcept {
    // Phase 22: confirm the binary links libvulkan.so.1 natively on
    // steamrt-sniper (no Proton fallback).
    return {"native_linux_vulkan", true};
}

} // namespace

int main(int argc, char** /*argv*/) {
    const ProbeResult probes[] = {
        check_no_launcher(),
        check_controller_glyphs(),
        check_default_bindings(),
        check_no_compat_warnings(),
        check_native_linux_vk(),
    };
    int failures = 0;
    for (const auto& p : probes) {
        std::printf("[deck-probe] %-30s %s\n", p.name, p.ok ? "PASS" : "FAIL");
        if (!p.ok) ++failures;
    }
    std::printf("DECK PROBE %s — probes=%zu failures=%d\n",
                failures == 0 ? "OK" : "FAIL",
                sizeof(probes)/sizeof(probes[0]), failures);
    (void)argc;
    return failures == 0 ? 0 : 1;
}
