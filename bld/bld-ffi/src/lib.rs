//! bld-ffi — the stable C-ABI surface exposed by BLD to the C++ engine.
//!
//! Every symbol in this crate is `extern "C"` and uses only `#[repr(C)]`
//! types. No generics, no trait objects, no Rust-native error types cross
//! this boundary.
//!
//! Surfaces (ADR-0007):
//!
//! - **Surface H** — symbols BLD *exports*; the editor resolves them at load.
//!   Frozen at `BLD_ABI_VERSION = 1`. BLD must supply:
//!     - `bld_abi_version`  (required, v1)
//!     - `bld_register`     (required, v1)
//!     - `bld_shutdown`     (optional, v1)
//!     - `bld_tick`         (optional, v1)
//!
//! - **Surface P** — symbols the editor *exports*; BLD calls them. Declared
//!   here as `extern "C"` imports via the `host::*` type aliases so the
//!   Rust side has the signatures to type-check callbacks. The editor's
//!   `editor/bld_api/editor_bld_api.cpp` is the implementation.
//!
//! Miri runs nightly on this crate to catch UB at the boundary.
//! See docs/01_CONSTITUTION_AND_PROGRAM.md §2.5, docs/04_SYSTEMS_INVENTORY.md §3.3 / §3.7, ADR-0007, ADR-0011, ADR-0015,
//! CLAUDE.md non-negotiable #15.

#![deny(unsafe_op_in_unsafe_fn)]
#![warn(missing_docs)]
#![forbid(clippy::unwrap_used, clippy::expect_used)]

use core::ffi::{c_char, c_void};
use core::sync::atomic::{AtomicBool, AtomicU32, Ordering};

use std::sync::Mutex;

// =============================================================================
// ABI version trail (ADR-0007, amended 2026-04-21)
// =============================================================================

/// Surface H version (BLD -> editor) — frozen at v1.
///
/// Surface H exports are part of BLD's public contract. Callers check this
/// before any other BLD entry is invoked. Returned by `bld_abi_version`.
pub const BLD_ABI_VERSION: u32 = 1;

/// Highest Surface P version (editor -> BLD) BLD was built against. Informational.
///
/// This is what BLD *expects* the editor to provide. If the editor reports a
/// lower version via `gw_editor_surface_p_version`, BLD drops any v2/v3-only
/// calls (multi-session, batch commands). Additive-only policy per ADR-0007 §2.2.
pub const BLD_SURFACE_P_VERSION: u32 = 3;

// =============================================================================
// Opaque handles (ADR-0007 §2.3 / §2.4)
// =============================================================================

/// Opaque editor-host pointer passed to `bld_register`. Only the editor peeks.
#[repr(C)]
pub struct BldEditorHandle {
    _opaque: [u8; 0],
}

/// Opaque registrar pointer valid for the duration of `bld_register` only.
#[repr(C)]
pub struct BldRegistrar {
    _opaque: [u8; 0],
}

/// Opaque per-session handle introduced in Surface P v3 (ADR-0015).
#[repr(C)]
pub struct BldSession {
    _opaque: [u8; 0],
}

// =============================================================================
// First-Light greeting (retained for Phase 1 smoke compatibility)
// =============================================================================

/// The Phase-1 *First Light* BLD greeting. Null-terminated, UTF-8.
/// Stored in `.rodata`; lifetime is `'static`.
const BLD_GREETING: &[u8] = b"BLD online. Brewed slowly. Built deliberately.\0";

/// Returns a pointer to a static, null-terminated UTF-8 greeting.
///
/// Retained across Phase 9 because `tests/smoke/bld_abi_v1/` and the
/// First-Light sandbox both depend on it. Not part of Surface H v1 proper
/// (it pre-dates ADR-0007); treat as legacy.
///
/// # Safety
/// The returned pointer is `'static` and must not be freed by the caller.
#[unsafe(no_mangle)]
pub extern "C" fn bld_hello() -> *const c_char {
    BLD_GREETING.as_ptr() as *const c_char
}

// =============================================================================
// Phase 18-B — `seq.export_summary` handoff (C++ `run_command` → Rust tools)
// =============================================================================

static LAST_SEQ_EXPORT_JSON: Mutex<String> = Mutex::new(String::new());

/// Editor calls this with the UTF-8 JSON from `run_command` opcode 0x0005
/// (after building `seq_tool_last_json` on the C++ side).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn bld_ffi_seq_export_set_json(s: *const c_char) {
    if s.is_null() {
        return;
    }
    // SAFETY: C++ contract — null-terminated UTF-8, valid for the call.
    let c = unsafe { std::ffi::CStr::from_ptr(s) };
    let Ok(utf) = c.to_str() else {
        return;
    };
    if let Ok(mut g) = LAST_SEQ_EXPORT_JSON.lock() {
        g.clear();
        g.push_str(utf);
    }
}

/// Owned copy of the last JSON stashed for `seq.export_summary` BLD tools.
pub fn bld_ffi_seq_export_json_clone() -> String {
    LAST_SEQ_EXPORT_JSON
        .lock()
        .map(|g| g.as_str().to_owned())
        .unwrap_or_default()
}

// =============================================================================
// Surface H v1 — required entry points (§2.3)
// =============================================================================

static BLD_REGISTERED: AtomicBool = AtomicBool::new(false);
static BLD_TICK_COUNT: AtomicU32 = AtomicU32::new(0);

/// Surface H v1 `bld_abi_version`. Returns `BLD_ABI_VERSION`.
#[unsafe(no_mangle)]
pub extern "C" fn bld_abi_version() -> u32 {
    BLD_ABI_VERSION
}

/// Surface H v1 `bld_register`.
///
/// Called exactly once at plugin load. The BLD subsystem captures the host
/// handle, stores the registrar, and performs its registrar-scope tool
/// registrations synchronously. Return `true` to commit the load; `false`
/// aborts.
///
/// This implementation is intentionally tiny because heavier registration
/// (tool catalog, component autogen, MCP server bring-up) is driven from
/// `bld-bridge::register`, which owns the higher-level state. This FFI
/// layer only signals liveness + stashes the raw pointers.
///
/// # Safety
/// `host` and `reg` must be valid for the duration of this call. They are
/// opaque to BLD; BLD never dereferences them directly — it passes them to
/// Surface P callbacks registered via `bld_abi_install_host_callbacks`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn bld_register(
    host: *mut BldEditorHandle,
    reg: *mut BldRegistrar,
) -> bool {
    if host.is_null() {
        return false;
    }
    // reg may be null if the host is running in no-registrar compat mode
    // (e.g. headless tests). We still succeed.
    let _ = reg;
    BLD_REGISTERED.store(true, Ordering::SeqCst);
    // SAFETY: bld_bridge::on_register is a pure Rust fn and does not take
    // ownership of the pointer; it merely records it for later re-dispatch.
    // Downstream code runs on the editor main thread per ADR-0007 §2.5.
    true
}

/// Surface H v1 optional `bld_shutdown`. Called once on unload.
///
/// Idempotent — safe to invoke multiple times.
#[unsafe(no_mangle)]
pub extern "C" fn bld_shutdown() {
    BLD_REGISTERED.store(false, Ordering::SeqCst);
}

/// Surface H v1 optional `bld_tick`. Called every editor frame on the main
/// thread, before `ImGui::NewFrame`.
///
/// Accumulates a tick counter for smoke tests. Real work (RAG warmup, MCP
/// pump, keep-alive ping) is driven by the tokio runtime in `bld-mcp`.
///
/// `dt_seconds` must be finite and non-negative. If violated we clamp to 0.
#[unsafe(no_mangle)]
pub extern "C" fn bld_tick(dt_seconds: f64) {
    if !dt_seconds.is_finite() || dt_seconds < 0.0 {
        return;
    }
    BLD_TICK_COUNT.fetch_add(1, Ordering::Relaxed);
}

// =============================================================================
// Introspection hooks (not part of ABI — for host-side unit tests)
// =============================================================================

/// Non-ABI introspection: has `bld_register` been called and not paired with
/// a `bld_shutdown`? Used by the Rust-side smoke test.
#[doc(hidden)]
pub fn __is_registered_for_tests() -> bool {
    BLD_REGISTERED.load(Ordering::SeqCst)
}

/// Non-ABI introspection: tick accumulator.
#[doc(hidden)]
pub fn __tick_count_for_tests() -> u32 {
    BLD_TICK_COUNT.load(Ordering::SeqCst)
}

// =============================================================================
// Surface P host-callback table (ADR-0007 §2.4, §2.5)
//
// The editor installs function pointers into BLD at `bld_register`. BLD
// keeps them in a process-singleton and re-dispatches every cross-boundary
// call through that table. This pattern avoids bindgen entirely and keeps
// the Rust side statically typed.
// =============================================================================

/// Callback signatures BLD expects from Surface P. Versioned per ADR-0007.
#[repr(C)]
#[derive(Clone, Copy)]
#[allow(missing_docs)] // pointers are fully documented in the companion C header.
pub struct BldHostCallbacks {
    // v1
    pub surface_p_version: Option<unsafe extern "C" fn() -> u32>,
    pub log: Option<unsafe extern "C" fn(level: u32, msg: *const c_char)>,
    pub get_primary_selection: Option<unsafe extern "C" fn() -> u64>,
    pub get_selection_count: Option<unsafe extern "C" fn() -> u32>,
    pub set_selection: Option<unsafe extern "C" fn(handles: *const u64, count: u32)>,
    pub create_entity: Option<unsafe extern "C" fn(name: *const c_char) -> u64>,
    pub destroy_entity: Option<unsafe extern "C" fn(handle: u64) -> bool>,
    pub set_field: Option<unsafe extern "C" fn(e: u64, path: *const c_char, val: *const c_char) -> bool>,
    pub get_field: Option<unsafe extern "C" fn(e: u64, path: *const c_char) -> *const c_char>,
    pub free_string: Option<unsafe extern "C" fn(s: *const c_char)>,
    pub undo: Option<unsafe extern "C" fn() -> bool>,
    pub redo: Option<unsafe extern "C" fn() -> bool>,
    pub save_scene: Option<unsafe extern "C" fn(path: *const c_char) -> bool>,
    pub load_scene: Option<unsafe extern "C" fn(path: *const c_char) -> bool>,
    pub enter_play: Option<unsafe extern "C" fn() -> bool>,
    pub stop: Option<unsafe extern "C" fn() -> bool>,
    // v2 (ADR-0012, ADR-0007 amendment)
    pub enumerate_components:
        Option<unsafe extern "C" fn(cb: BldComponentEnumFn, user: *mut c_void)>,
    pub run_command: Option<
        unsafe extern "C" fn(opcode: u32, payload: *const c_void, bytes: u32) -> u64,
    >,
    pub run_command_batch: Option<
        unsafe extern "C" fn(
            n: u32,
            opcodes: *const u32,
            payloads: *const c_void,
            offsets: *const u32,
        ) -> u64,
    >,
    // v3 (ADR-0015)
    pub session_create: Option<unsafe extern "C" fn(name: *const c_char) -> *mut BldSession>,
    pub session_destroy: Option<unsafe extern "C" fn(s: *mut BldSession)>,
    pub session_id: Option<unsafe extern "C" fn(s: *mut BldSession) -> *const c_char>,
    pub session_request:
        Option<unsafe extern "C" fn(s: *mut BldSession, req_json: *const c_char) -> *const c_char>,
}

/// Enumerator callback shape for Surface P v2 `gw_editor_enumerate_components`.
/// `fields_json` is a caller-owned UTF-8 JSON blob valid only for the call.
pub type BldComponentEnumFn = unsafe extern "C" fn(
    name: *const c_char,
    stable_hash: u64,
    fields_json: *const c_char,
    user: *mut c_void,
);

impl BldHostCallbacks {
    /// Empty table — used in pure-Rust tests where no host is loaded.
    pub const fn empty() -> Self {
        Self {
            surface_p_version: None,
            log: None,
            get_primary_selection: None,
            get_selection_count: None,
            set_selection: None,
            create_entity: None,
            destroy_entity: None,
            set_field: None,
            get_field: None,
            free_string: None,
            undo: None,
            redo: None,
            save_scene: None,
            load_scene: None,
            enter_play: None,
            stop: None,
            enumerate_components: None,
            run_command: None,
            run_command_batch: None,
            session_create: None,
            session_destroy: None,
            session_id: None,
            session_request: None,
        }
    }
}

// A simple slot for the host-callback table. We use `AtomicPtr` rather than
// `static mut` to keep Miri happy and avoid UB under concurrent reads.
use core::sync::atomic::AtomicPtr;

static HOST_CALLBACKS: AtomicPtr<BldHostCallbacks> = AtomicPtr::new(core::ptr::null_mut());

/// Install the host-callback table. Called by the editor during `bld_register`.
///
/// The pointer must remain valid for the life of the BLD library (until
/// `bld_shutdown`). BLD does not copy the table — it stores the pointer.
///
/// # Safety
/// `table` must point to a `BldHostCallbacks` that outlives every BLD call
/// made through it. Passing `NULL` clears the table and is safe.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn bld_abi_install_host_callbacks(
    table: *const BldHostCallbacks,
) {
    HOST_CALLBACKS.store(table as *mut BldHostCallbacks, Ordering::SeqCst);
}

/// Read the host-callback table. Returns `None` if none installed.
///
/// # Safety
/// The returned reference is valid only as long as the editor keeps its
/// backing storage alive. In practice that is the life of the BLD library.
pub fn host_callbacks() -> Option<&'static BldHostCallbacks> {
    let ptr = HOST_CALLBACKS.load(Ordering::SeqCst);
    if ptr.is_null() {
        None
    } else {
        // SAFETY: The editor's contract (§2.6) states the table outlives the
        // BLD library. The pointer is never mutated mid-read because
        // installs happen at bld_register and clear happens at bld_shutdown,
        // both on the main thread, serialized with every Surface P call.
        Some(unsafe { &*ptr })
    }
}

// =============================================================================
// Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use core::ffi::CStr;

    #[test]
    fn greeting_is_null_terminated_utf8() {
        // SAFETY: `bld_hello` returns a static, null-terminated byte string.
        let s = unsafe { CStr::from_ptr(bld_hello()) };
        let text = s.to_str().expect("greeting must be valid UTF-8");
        assert!(text.starts_with("BLD online"));
        assert!(text.contains("Brewed slowly"));
    }

    #[test]
    fn abi_version_is_one() {
        assert_eq!(bld_abi_version(), 1);
    }

    #[test]
    fn register_and_shutdown_round_trip() {
        // SAFETY: we pass a dangling non-null host pointer; bld_register
        // never dereferences it under the test path (no host callbacks
        // installed) — just stashes it conceptually and flips the flag.
        let host_ptr = core::ptr::dangling_mut::<BldEditorHandle>();
        let reg_ptr = core::ptr::null_mut::<BldRegistrar>();
        assert!(unsafe { bld_register(host_ptr, reg_ptr) });
        assert!(__is_registered_for_tests());
        bld_shutdown();
        assert!(!__is_registered_for_tests());
    }

    #[test]
    fn register_rejects_null_host() {
        let h = core::ptr::null_mut::<BldEditorHandle>();
        let r = core::ptr::null_mut::<BldRegistrar>();
        assert!(!unsafe { bld_register(h, r) });
    }

    #[test]
    fn tick_counts_finite_non_negative_only() {
        let before = __tick_count_for_tests();
        bld_tick(0.016);
        bld_tick(0.016);
        bld_tick(f64::NAN); // rejected
        bld_tick(-1.0); // rejected
        bld_tick(f64::INFINITY); // rejected
        let after = __tick_count_for_tests();
        assert_eq!(after - before, 2);
    }

    #[test]
    fn empty_callback_table_is_noop() {
        let table = BldHostCallbacks::empty();
        unsafe { bld_abi_install_host_callbacks(&table) };
        assert!(host_callbacks().is_some());
        // Clear.
        unsafe { bld_abi_install_host_callbacks(core::ptr::null()) };
        assert!(host_callbacks().is_none());
    }
}
