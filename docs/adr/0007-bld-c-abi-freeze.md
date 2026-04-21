# ADR 0007 — BLD C-ABI registration surface freeze

- **Status:** Accepted
- **Date:** 2026-04-20 (late-night; Phase-7 fullstack Path-A)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** the informal `editor/bld_api/editor_bld_api.hpp` header comment rules ("Never remove or reorder existing entries — only add at the tail") — lifted to a formal ADR with versioning rigor.
- **Superseded by:** —
- **Related:** `docs/05 §Phase 7` (Editor v0.1) and `§Phase 9` (BLD); `docs/02_SYSTEMS_AND_SIMULATIONS.md §BLD`; ADR-0004 (ECS-World — `gw_editor_*` functions route through it); ADR-0005 (CommandStack — `gw_editor_undo/redo` route through it); ADR-0006 (serialization — `gw_editor_save_scene/load_scene` route through it); `CLAUDE.md` non-negotiables #8 (`std::string_view` never to C API — doubly relevant here), #20 (doctrine first).

## 1. Context

Phase 7 (Editor v0.1) week 042 commits to "BLD C-ABI registration surface freeze + one-call-site smoke test." Phase 9 (Brewed Logic, Rust, weeks 048–059) is the actual BLD implementation. The ABI between them needs to be locked *before* Phase 9 starts so BLD can be written against a stable host surface, and so we can't accidentally re-litigate the shape every time an editor feature lands.

The relationship between the two halves:

- **Editor is the host.** It loads `brewed_logic.dll` / `libbrewed_logic.so` on startup (or on user demand; both patterns land in Phase 9). Once loaded, the editor resolves a small set of well-known BLD entry points (`bld_register`, `bld_register_tool`, `bld_register_widget`, …) and calls them.
- **BLD is the plugin.** On `bld_register`, BLD performs its own initialization, then calls back into the editor using `gw_editor_*` exports to register tools, widgets, menu items, hot-reload watchers, MCP endpoints, and RAG hooks. BLD is a Rust crate with a thin C-ABI shim; the shim does nothing the editor can't do — it's just the language boundary.
- **Ongoing communication** happens in both directions: editor → BLD on events (build requested, scene changed, selection changed), BLD → editor on registered-callback invocation (user ran a tool, a widget wants a redraw).

Today, `editor/bld_api/editor_bld_api.{hpp,cpp}` has the editor-export half partially sketched but informally versioned. The BLD-import half (`bld_register*` entry points the editor resolves in `brewed_logic.dll`) doesn't exist yet. Both halves need to be pinned before Phase 9.

An early existing file, `editor/bld_bridge/hello.{hpp,cpp}`, is the *First-Light* smoke: a single `gw_bld_hello()` round-trip proving the dynamic-library plumbing works. It predates this ADR and is retired when Phase-9 begins; we reference it here only to explain why the current tree already has a BLD bridge directory that is *not* the ABI.

## 2. Decision

### 2.1 The two surfaces

**Surface H (Host → BLD).** Symbols the editor looks up in `brewed_logic.dll` via `gw::platform::DynamicLibrary::resolve(name)`. BLD **must** export them; failure to resolve any required symbol means BLD is not loaded.

**Surface P (Plugin → Host).** `extern "C"` functions exported by `gw_editor` (the executable — symbols resolved via the process handle on dlopen/LoadLibrary semantics). BLD links against these at load time via `GetProcAddress` (Windows) or by letting the dynamic loader resolve symbols in the host executable (Linux, via `-rdynamic` / explicit dlopen). *This* is the surface partially present in `editor/bld_api/editor_bld_api.hpp`.

Both surfaces are C ABI. Neither surface uses C++ types across the boundary. Neither surface uses `std::string_view` (NN #8) — strings are `const char*` with caller-owns-or-free-via-callback semantics documented per-function.

### 2.2 Versioning — single monotonic `BLD_ABI_VERSION`

Every ABI item is tagged with the `BLD_ABI_VERSION` that introduced it. On load:

1. Editor calls `bld_abi_version()` → returns the ABI version BLD was built against (u32).
2. Editor compares with its own compiled-in `BLD_ABI_VERSION_HOST`.
3. Policy:
   - `BLD.version == Host.version` → load normally.
   - `BLD.version < Host.version` → load in **compat mode**: missing-from-BLD optional registration entry points are ignored; required entry points must still resolve. Log which newer-than-BLD items were skipped.
   - `BLD.version > Host.version` → **refuse to load**. Log that the host is older than the plugin. BLD must not assume newer host features.

`BLD_ABI_VERSION = 1` ships with this ADR. Every additive change (new registration entry point, new `gw_editor_*` function) bumps the version. **Removals and signature changes are forbidden without a version bump and a migration plan in a follow-up ADR** — this is the "only add at the tail" rule, lifted to ADR-gate strength.

### 2.3 Surface H — the BLD-side entry points (frozen at v1)

```c
/* bld/include/bld_register.h — ships with the editor, consumed by BLD.
 * C ABI. BLD implements these; the editor looks them up. */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations of opaque handles passed to BLD. */
typedef struct BldEditorHandle BldEditorHandle;   /* opaque; carries host globals */
typedef struct BldRegistrar    BldRegistrar;      /* opaque; BLD calls bld_registrar_* via host */

/* Required entry points (must resolve or BLD fails to load).
 * bld_abi_version() must be defined before any other BLD entry is called. */

/* Introduced in v1. */ uint32_t bld_abi_version(void);

/* Introduced in v1. Called once on BLD load. `host` is owned by the editor
 * and valid for the life of the BLD library. `reg` is the registrar handle
 * BLD uses to declare its tools/widgets synchronously during registration.
 * Returns true on success; false aborts load. */
/* Introduced in v1. */ bool bld_register(BldEditorHandle* host, BldRegistrar* reg);

/* Optional entry points (missing == feature not provided by BLD).
 * Missing optional entry points in compat mode are tolerated. */

/* Introduced in v1. Called once on BLD unload or editor shutdown.
 * Ordering: after the last host → BLD call, before the library is freed. */
/* Introduced in v1. */ void bld_shutdown(void);

/* Introduced in v1. Periodic tick for BLD housekeeping (async RAG warmup,
 * hot-reload polling). Called on the editor main thread before ImGui NewFrame.
 * Safe to leave unimplemented; editor won't call a missing symbol. */
/* Introduced in v1. */ void bld_tick(double dt_seconds);

#ifdef __cplusplus
}
#endif
```

### 2.4 Surface P — the editor-side exports (frozen at v1, superset of what's in the tree today)

Refer to `editor/bld_api/editor_bld_api.hpp` as the authoritative header. The ADR amendment to the existing surface is:

**Added in v1 (not in the tree today, but part of the frozen v1 surface):**

```c
/* Registration helpers — called by BLD during bld_register().
 * `reg` is the BldRegistrar the editor passed to bld_register. */

/* Register a named tool. Invoked from the editor's Tools menu or via the
 * `gw_editor_run_tool` API. `fn` is called on the editor main thread.
 * Returns a non-zero tool id, or 0 on failure (duplicate name, etc.).
 * `name` and `tooltip` must be UTF-8; the editor copies them (not retained).
 * Icons are optional; pass nullptr if unused. */
typedef void (*BldToolFn)(void* user_data);

GW_EDITOR_API uint32_t bld_registrar_register_tool(BldRegistrar* reg,
                                                    const char* id,       /* stable, "org.brewedlogic.mytool" */
                                                    const char* name,
                                                    const char* tooltip_utf8,
                                                    const char* icon_name,/* or null */
                                                    BldToolFn   fn,
                                                    void*       user_data);

/* Register a custom inspector widget for a component type key.
 * `component_hash` must match ComponentRegistry.stable_hash (ADR-0004 §2.4).
 * `draw_fn` is called inside ImGui BeginChild for the component block;
 * it MUST NOT begin/end its own ImGui window. */
typedef void (*BldWidgetFn)(void* component_ptr, void* user_data);

GW_EDITOR_API uint32_t bld_registrar_register_widget(BldRegistrar* reg,
                                                      uint64_t    component_hash,
                                                      BldWidgetFn draw_fn,
                                                      void*       user_data);

/* Tool invocation surface exposed to external automation (the editor's own
 * menu uses the same path). Names are BLD-registered ids. Returns false if
 * the tool id is unknown. Any return value / error propagation is BLD's
 * responsibility — they can queue an error into the editor log via the
 * existing log surface (gw_editor_log_error — v1). */
GW_EDITOR_API bool gw_editor_run_tool(const char* tool_id);

/* Logging surface BLD uses to report errors / warnings / info into the
 * editor's console panel. Level: 0=info, 1=warn, 2=error. */
GW_EDITOR_API void gw_editor_log(uint32_t level, const char* message_utf8);
GW_EDITOR_API void gw_editor_log_error(const char* message_utf8);  /* convenience */
```

**Retained from the existing header (see the file) at v1:**

- `gw_editor_version()` — **renamed semantically:** returns the **editor** build string; BLD uses `bld_abi_version()` (Surface H) for negotiation. Editor-version string is informational only.
- Selection: `gw_editor_get_primary_selection`, `gw_editor_get_selection_count`, `gw_editor_set_selection`.
- Entity management: `gw_editor_create_entity`, `gw_editor_destroy_entity`.
- Field access: `gw_editor_set_field`, `gw_editor_get_field`, `gw_free_string`.
- Command stack: `gw_editor_undo`, `gw_editor_redo`, `gw_editor_command_stack_summary`.
- Scene: `gw_editor_save_scene`, `gw_editor_load_scene`.
- PIE: `gw_editor_enter_play`, `gw_editor_stop`.

**All `const char*` outputs** use the `gw_free_string(const char*)` free function (retained). Caller passes back the pointer that was returned; editor frees with its own allocator. Missing in the current header: a matching convention for **callback parameters** — BLD callbacks may not retain string pointers across calls. We document that here and enforce it in the smoke-test pair.

### 2.5 Threading model

- All Surface-P calls execute on whatever thread BLD calls from. The editor's implementation **must** be thread-safe for every export, either by taking a lock or by posting work to the main thread and waiting. The existing comment in `editor_bld_api.hpp` ("mutations are queued for main-thread execution") is upgraded to an ADR requirement.
- All Surface-H calls are made **only** from the editor main thread. BLD can assume no concurrent calls into its exports.
- `bld_tick` runs on the editor main thread before `ImGui::NewFrame`.

### 2.6 Memory / lifetime contracts

- **Input strings** (`const char* name` etc.) are valid only for the duration of the call. The callee must copy if it needs to retain.
- **Output strings** returned by `gw_editor_get_field`, `gw_editor_command_stack_summary`, etc. are caller-owned; caller frees via `gw_free_string`.
- **Opaque handle pointers** (`BldEditorHandle*`, `BldRegistrar*`) are owned by the editor; BLD must not free them.
- **Callbacks registered via `bld_registrar_register_*`** live until either BLD unloads or `bld_shutdown` is called; unregistration at runtime is not supported in v1 (deferred to v2 if needed).
- **`void* user_data`** pointers passed to callbacks are owned by BLD; the editor never dereferences or frees them. On BLD unload the editor drops the pointer without calling a "free user_data" hook (v2 addition if anyone needs it).

### 2.7 The Phase-7 smoke-test site

One end-to-end call path exercises every v1 section without needing a real BLD implementation:

1. Build a `tests/smoke/bld_abi_v1/` mini-dylib that implements the Surface-H entry points in C++ (BLD is Rust long-term; for the smoke we need any language).
2. The smoke dylib's `bld_register` calls `bld_registrar_register_tool(reg, "test.hello", "Hello", "Say hi", nullptr, &say_hi_fn, nullptr)`.
3. `gw_editor` loads the dylib, calls `bld_abi_version`, calls `bld_register`, then calls `gw_editor_run_tool("test.hello")` which invokes `say_hi_fn` which calls `gw_editor_log_error("bld_abi v1 round-trip OK")`.
4. Test asserts the log surface received that message.

That single test covers: symbol resolution, version negotiation, registrar round-trip, tool invocation, host-log callback. It *does not* exercise widgets or the full set of `gw_editor_*` calls, but it locks the shape by keeping the smoke dylib as a consumer that would break if anyone silently changed the ABI.

### 2.8 Out of scope for v1 (explicitly)

- **Async tool invocation / job integration.** Tools run synchronously on the editor main thread in v1. Async tools are a v2 addition.
- **Streaming / large-payload transfer** between host and plugin (BLD's RAG corpus, long-running bg computes). v1 passes control only; BLD manages its own heaps.
- **Hot-reload of BLD itself.** v1 supports load-on-startup and unload-on-shutdown. Reload-during-session lands with Phase-9 hot-reload work, and *that* is where we'd add `bld_on_reload` to Surface H.
- **MCP endpoint surface.** BLD's MCP endpoints live inside BLD's own HTTP server; the ABI only needs to know "BLD is alive and can take requests." That's already covered by `bld_tick`.
- **Python / JavaScript scripting interop.** BLD is Rust. If we ever bind Python, that's a new ADR.

## 3. Consequences

### Immediate (this commit block)

- `bld/include/bld_register.h` lands (C header; symlinked or copied into BLD's Rust crate at Phase-9 start).
- `editor/bld_api/editor_bld_api.hpp` gains the new v1 additions (registrar/tool/widget/log surfaces).
- `editor/bld_api/editor_bld_api.cpp` ships stub implementations — each function routes into the right subsystem (selection / ECS-World / CommandStack / serialization) per the ADR-0004/0005/0006 plumbing. Where a subsystem isn't yet wired (today), the function returns false / 0 / empty with a log line; Phase-7 implementation work fills in as each subsystem lands.
- `tests/smoke/bld_abi_v1/` smoke dylib + test target.
- The existing `editor/bld_bridge/hello.{hpp,cpp}` is kept as a marker for First-Light but *not* part of the v1 ABI surface; a comment in that file points at this ADR.

### Short term (Phase 9 entry)

- BLD builds against `bld/include/bld_register.h` as its C-side contract.
- BLD implements `bld_register` / `bld_abi_version` / (optionally) `bld_shutdown` / `bld_tick`.
- First BLD feature (probably a `bld.compile` tool) registers via `bld_registrar_register_tool`; the smoke test evolves into a real feature test.

### Long term

- Every v1 → v2 change bumps `BLD_ABI_VERSION`. Removed functions get a `GW_EDITOR_API_DEPRECATED(v)` marker and live one version beyond removal for compat mode.
- When hot-reload of BLD lands (Phase 9 late), Surface H gains `bld_on_reload_begin` / `bld_on_reload_end` at v2.
- If BLD ever needs to drive the editor from a thread other than main (unlikely; it calls back from editor callbacks, which already run on main), a `gw_editor_post_to_main(fn, user_data)` addition covers it at v2+.

### Alternatives considered

1. **Flat C ABI with no registrar, plugins register by side-effect at load time** (e.g. `bld_register_tools` is itself a static list consumed by the editor). Rejected — registrar-based is strictly more flexible, and the registrar pattern maps cleanly to BLD-authored runtime registrations we know are coming (widgets bound to user-authored components).

2. **RPC / message-queue based (protobuf over unix-socket).** Rejected for v1 as wildly over-engineered for an in-process plugin. Revisit if BLD ever needs to run out-of-process (sandbox isolation for untrusted plugins — possibly relevant to the eventual mod-plugin story, which is *not* BLD).

3. **Expose the entire C++ editor surface directly via Rust bindings (bindgen / cxx).** Rejected — binds BLD to the editor's C++ ABI, which varies per compiler/STL. A hand-curated C ABI is stable across compilers, which is the whole point of this ADR.

4. **Version per-function instead of per-surface.** Rejected. A single monotonic `BLD_ABI_VERSION` + "added in v_N" annotations is simpler to reason about and matches how Khronos versions the Vulkan API.

5. **No formal versioning; "trust the developer" (the current state).** Rejected — this is exactly what the Phase-3 drift (P1-11) was caused by. Doctrine first, enforced.

## 4. References

- `editor/bld_api/editor_bld_api.hpp` — existing partial surface
- `editor/bld_bridge/hello.{hpp,cpp}` — First-Light smoke (not ABI)
- `docs/05 §Phase 7 week 042` — freeze-at-v1 commitment
- `docs/05 §Phase 9` — BLD build-out
- `docs/02_SYSTEMS_AND_SIMULATIONS.md §BLD`
- ADR-0004 / ADR-0005 / ADR-0006 — the subsystems the exports route into
- `CLAUDE.md` non-negotiables #8, #20

---

*Drafted by Claude Opus 4.7 on 2026-04-20 late-night as part of the Phase-7 fullstack Path-A push. Doctrine lands before code per `CLAUDE.md` non-negotiable #20.*
