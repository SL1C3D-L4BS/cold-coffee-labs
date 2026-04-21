/* bld/include/bld_register.h
 * Surface H  BLD plugin entry points + host-callback table (Surface P v1-v3).
 * Frozen at BLD_ABI_VERSION = 1 for Surface H; tracks Surface P versions
 * v1/v2/v3 for host-side additions. See docs/adr/0007-bld-c-abi-freeze.md
 * (amended 2026-04-21) and the Phase-9 ADRs 0010-0016.
 *
 * Rules:
 *   - Every item is tagged with the version that introduced it.
 *   - Additions happen at the tail; never reorder, never remove.
 *   - Signature changes require a version bump and a migration plan ADR.
 *   - Strings are `const char*`, UTF-8, caller-owned, not retained unless
 *     explicitly stated. std::string_view never crosses this boundary
 *     (CLAUDE.md non-negotiable #8).
 *   - Surface H calls run on the editor main thread; BLD may assume no
 *     concurrent invocation.
 *   - Surface P calls are thread-safe on the editor; implementations may
 *     queue work to the main thread internally.
 */

#ifndef GW_BLD_REGISTER_H_
#define GW_BLD_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Monotonic Surface-H ABI version. Frozen at 1 for Phase 9. */
#define BLD_ABI_VERSION 1u

/* Highest Surface-P version BLD was built against (informational). */
#define BLD_SURFACE_P_VERSION 3u

/* -------------------------------------------------------------------------
 * Opaque handles.
 * ------------------------------------------------------------------------- */
typedef struct BldEditorHandle BldEditorHandle;
typedef struct BldRegistrar    BldRegistrar;
typedef struct BldSession      BldSession;            /* Surface P v3 */

/* -------------------------------------------------------------------------
 * REQUIRED Surface H v1 entry points (BLD must export).
 * ------------------------------------------------------------------------- */

/* v1 */ uint32_t bld_abi_version(void);

/* v1. Called exactly once on plugin load. Return true to commit. */
bool bld_register(BldEditorHandle* host, BldRegistrar* reg);

/* -------------------------------------------------------------------------
 * OPTIONAL Surface H v1 entry points.
 * ------------------------------------------------------------------------- */

/* v1. Called once on plugin unload. */
void bld_shutdown(void);

/* v1. Called every editor frame, main thread, before ImGui::NewFrame. */
void bld_tick(double dt_seconds);

/* -------------------------------------------------------------------------
 * Surface-P host-callback table (ADR-0007 §2.4, amended 2026-04-21).
 *
 * The editor populates this struct and passes it to BLD via
 * `bld_abi_install_host_callbacks`. BLD stores the pointer and routes
 * every cross-boundary call through it. Any field may be NULL if the host
 * does not implement that entry (BLD checks before invoking).
 * ------------------------------------------------------------------------- */

/* Enumerator callback invoked by `enumerate_components` (Surface P v2). */
typedef void (*BldComponentEnumFn)(const char* name,
                                    uint64_t    stable_hash,
                                    const char* fields_json,
                                    void*       user);

typedef struct BldHostCallbacks {
    /* v1 */
    uint32_t (*surface_p_version)(void);
    void     (*log)(uint32_t level, const char* message_utf8);
    uint64_t (*get_primary_selection)(void);
    uint32_t (*get_selection_count)(void);
    void     (*set_selection)(const uint64_t* handles, uint32_t count);
    uint64_t (*create_entity)(const char* name);
    bool     (*destroy_entity)(uint64_t handle);
    bool     (*set_field)(uint64_t e, const char* path, const char* val);
    const char* (*get_field)(uint64_t e, const char* path);
    void     (*free_string)(const char* s);
    bool     (*undo)(void);
    bool     (*redo)(void);
    bool     (*save_scene)(const char* path);
    bool     (*load_scene)(const char* path);
    bool     (*enter_play)(void);
    bool     (*stop)(void);

    /* v2 (ADR-0012, ADR-0007 amendment) */
    void     (*enumerate_components)(BldComponentEnumFn cb, void* user);
    uint64_t (*run_command)(uint32_t opcode, const void* payload, uint32_t bytes);
    uint64_t (*run_command_batch)(uint32_t opcode_count,
                                    const uint32_t* opcodes,
                                    const void* payloads,
                                    const uint32_t* payload_offsets);

    /* v3 (ADR-0015) */
    BldSession* (*session_create)(const char* display_name);
    void        (*session_destroy)(BldSession* s);
    const char* (*session_id)(BldSession* s);
    const char* (*session_request)(BldSession* s, const char* request_json);
} BldHostCallbacks;

/* Install the host-callback table. The pointer must outlive the BLD library
 * (conceptually: static storage in the editor). Passing NULL clears. */
void bld_abi_install_host_callbacks(const BldHostCallbacks* table);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* GW_BLD_REGISTER_H_ */
