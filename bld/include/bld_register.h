/* bld/include/bld_register.h
 * Surface H — Brewed Logic plugin entry points resolved by gw_editor.
 * Frozen at BLD_ABI_VERSION = 1. See docs/adr/0007-bld-c-abi-freeze.md.
 *
 * Rules (copied from the ADR so BLD authors don't need to cross-reference
 * every time they touch this header):
 *   - Every item is tagged with the BLD_ABI_VERSION that introduced it.
 *   - Additions happen at the tail; never reorder, never remove.
 *   - Signature changes require a version bump and a migration plan.
 *   - Strings are `const char*`, UTF-8, caller-owned, not retained unless
 *     explicitly stated. std::string_view never crosses this boundary
 *     (CLAUDE.md non-negotiable #8).
 *   - All Surface-H calls run on the editor main thread; BLD may assume
 *     no concurrent invocation of any BLD-exported symbol.
 */

#ifndef GW_BLD_REGISTER_H_
#define GW_BLD_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Monotonic ABI version — bumped for every additive change. */
#define BLD_ABI_VERSION 1u

/* -------------------------------------------------------------------------
 * Opaque handles. BLD must not peek inside. Defined (editor-side) in
 * editor/bld_api/editor_bld_api.cpp.
 * ------------------------------------------------------------------------- */
typedef struct BldEditorHandle BldEditorHandle;
typedef struct BldRegistrar    BldRegistrar;

/* -------------------------------------------------------------------------
 * REQUIRED entry points — BLD must export these; missing symbols fail load.
 * bld_abi_version() must be defined before any other BLD entry point is
 * called so the host can negotiate compatibility before handing out any
 * state.
 * ------------------------------------------------------------------------- */

/* Introduced in v1. */
uint32_t bld_abi_version(void);

/* Introduced in v1. Called exactly once on plugin load.
 *   host: owned by editor; valid for the life of the BLD library.
 *   reg : registrar handle used by BLD to declare tools/widgets during
 *         this synchronous call. Invalid once bld_register returns.
 * Return true to commit the load; false aborts and the host will unload. */
bool bld_register(BldEditorHandle* host, BldRegistrar* reg);

/* -------------------------------------------------------------------------
 * OPTIONAL entry points — host tolerates absence in compat mode.
 * Missing optional entry points are silently skipped; the host does not
 * log a warning for them (compat-mode is the common case cross-version).
 * ------------------------------------------------------------------------- */

/* Introduced in v1. Called exactly once on plugin unload or editor
 * shutdown, after the last host -> BLD call, before the library is freed. */
void bld_shutdown(void);

/* Introduced in v1. Called every editor frame on the main thread before
 * ImGui::NewFrame. Safe to leave unimplemented; host checks for nullptr. */
void bld_tick(double dt_seconds);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* GW_BLD_REGISTER_H_ */
