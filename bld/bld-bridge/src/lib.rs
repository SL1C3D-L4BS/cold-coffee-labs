//! bld-bridge — the Rust side of the C++ <-> Rust boundary.
//!
//! This crate re-exports the stable surface of `bld-ffi` and owns the small
//! amount of glue that turns raw Surface-P host callbacks into typed Rust
//! calls the rest of the BLD stack (`bld-agent`, `bld-mcp`, `bld-tools`)
//! can consume without touching `unsafe`.
//!
//! See ADR-0007 (amended 2026-04-21), ADR-0011, ADR-0015.

#![warn(missing_docs)]
#![forbid(clippy::unwrap_used, clippy::expect_used)]

pub use bld_ffi::{
    bld_abi_install_host_callbacks, bld_abi_version, bld_hello, bld_register, bld_shutdown,
    bld_tick, host_callbacks, BldComponentEnumFn, BldEditorHandle, BldHostCallbacks,
    BldRegistrar, BldSession, BLD_ABI_VERSION, BLD_SURFACE_P_VERSION,
};

use core::ffi::{c_char, CStr};

// =============================================================================
// Typed wrappers around the host-callback table.
//
// These are safe, allocating wrappers over the `unsafe extern "C"` pointers
// in `bld_ffi::BldHostCallbacks`. They return `Option` / `Result` and never
// panic. The `host` module is the *only* place the rest of BLD sees
// Surface-P; every other crate consumes it through this API.
// =============================================================================

/// Errors reported by the host bridge. Thin because most Surface-P calls
/// return bools — the wrapper translates `false` into a typed error.
#[derive(Debug, Clone)]
pub enum HostError {
    /// No host callback table installed (pure-Rust test context).
    NoHost,
    /// The specific callback is not implemented by the host.
    NotImplemented(&'static str),
    /// The host returned `false` / null / other failure sentinel.
    Failed(&'static str),
    /// The host returned a non-UTF-8 string.
    InvalidUtf8,
}

impl core::fmt::Display for HostError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            HostError::NoHost => f.write_str("no host callbacks installed"),
            HostError::NotImplemented(s) => write!(f, "host does not implement: {s}"),
            HostError::Failed(s) => write!(f, "host call failed: {s}"),
            HostError::InvalidUtf8 => f.write_str("host returned non-UTF-8 string"),
        }
    }
}

impl std::error::Error for HostError {}

fn host() -> Result<&'static BldHostCallbacks, HostError> {
    host_callbacks().ok_or(HostError::NoHost)
}

/// Convert a caller-owned C string from the host into an owned Rust string,
/// then hand back to the host so it can free through its allocator
/// (via `free_string`). This is the canonical `const char*` return pattern.
fn consume_host_cstr(ptr: *const c_char) -> Result<String, HostError> {
    if ptr.is_null() {
        return Err(HostError::Failed("null"));
    }
    // SAFETY: Host contract (ADR-0007 §2.6): returned strings are
    // caller-owned, null-terminated, UTF-8, valid until `free_string`.
    let c = unsafe { CStr::from_ptr(ptr) };
    let owned = c.to_str().map(|s| s.to_owned()).map_err(|_| HostError::InvalidUtf8);
    if let Some(free) = host_callbacks().and_then(|h| h.free_string) {
        // SAFETY: see above — `ptr` remains valid until we call free.
        unsafe { free(ptr) };
    }
    owned
}

// ---------------- v1 typed wrappers ----------------

/// Log a message to the editor console. Level 0=info, 1=warn, 2=error.
pub fn log(level: u32, message: &str) -> Result<(), HostError> {
    let h = host()?;
    let f = h.log.ok_or(HostError::NotImplemented("log"))?;
    let c = std::ffi::CString::new(message).map_err(|_| HostError::InvalidUtf8)?;
    // SAFETY: `c` is a valid null-terminated C string; `f` is the host callback.
    unsafe { f(level, c.as_ptr()) };
    Ok(())
}

/// Create an entity in the editor scene.
pub fn create_entity(name: Option<&str>) -> Result<u64, HostError> {
    let h = host()?;
    let f = h.create_entity.ok_or(HostError::NotImplemented("create_entity"))?;
    let c = match name {
        Some(n) => Some(std::ffi::CString::new(n).map_err(|_| HostError::InvalidUtf8)?),
        None => None,
    };
    let name_ptr = c.as_ref().map_or(core::ptr::null(), |s| s.as_ptr());
    // SAFETY: null-accepting per header contract; otherwise `c` is valid.
    let handle = unsafe { f(name_ptr) };
    if handle == 0 {
        return Err(HostError::Failed("create_entity returned 0"));
    }
    Ok(handle)
}

/// Destroy an entity.
pub fn destroy_entity(h: u64) -> Result<(), HostError> {
    let host = host()?;
    let f = host
        .destroy_entity
        .ok_or(HostError::NotImplemented("destroy_entity"))?;
    // SAFETY: plain scalar.
    let ok = unsafe { f(h) };
    if ok {
        Ok(())
    } else {
        Err(HostError::Failed("destroy_entity returned false"))
    }
}

/// Set a component field by reflection path.
pub fn set_field(entity: u64, path: &str, value_json: &str) -> Result<(), HostError> {
    let host = host()?;
    let f = host.set_field.ok_or(HostError::NotImplemented("set_field"))?;
    let p = std::ffi::CString::new(path).map_err(|_| HostError::InvalidUtf8)?;
    let v = std::ffi::CString::new(value_json).map_err(|_| HostError::InvalidUtf8)?;
    // SAFETY: both CStrings are valid, null-terminated, UTF-8.
    let ok = unsafe { f(entity, p.as_ptr(), v.as_ptr()) };
    if ok {
        Ok(())
    } else {
        Err(HostError::Failed("set_field returned false"))
    }
}

/// Read a component field as JSON string.
pub fn get_field(entity: u64, path: &str) -> Result<String, HostError> {
    let host = host()?;
    let f = host.get_field.ok_or(HostError::NotImplemented("get_field"))?;
    let p = std::ffi::CString::new(path).map_err(|_| HostError::InvalidUtf8)?;
    // SAFETY: CString valid; returned ptr is handed to consume_host_cstr.
    let ptr = unsafe { f(entity, p.as_ptr()) };
    consume_host_cstr(ptr)
}

/// Undo the most recent command on the editor CommandStack.
pub fn undo() -> Result<(), HostError> {
    let host = host()?;
    let f = host.undo.ok_or(HostError::NotImplemented("undo"))?;
    // SAFETY: nullary call.
    if unsafe { f() } { Ok(()) } else { Err(HostError::Failed("undo")) }
}

/// Redo.
pub fn redo() -> Result<(), HostError> {
    let host = host()?;
    let f = host.redo.ok_or(HostError::NotImplemented("redo"))?;
    // SAFETY: nullary call.
    if unsafe { f() } { Ok(()) } else { Err(HostError::Failed("redo")) }
}

// ---------------- v2 typed wrappers ----------------

/// Descriptor emitted once per reflected component during `enumerate_components`.
#[derive(Debug, Clone)]
pub struct ComponentDescriptor {
    /// Component name, e.g. `"TransformComponent"`.
    pub name: String,
    /// Stable BLAKE3-derived hash from the reflection registry.
    pub stable_hash: u64,
    /// Fields JSON: `[{"name":"position","type":"dvec3","offset":0}, ...]`.
    pub fields_json: String,
}

/// Enumerate all registered components via Surface P v2.
pub fn enumerate_components() -> Result<Vec<ComponentDescriptor>, HostError> {
    let h = host()?;
    let f = h
        .enumerate_components
        .ok_or(HostError::NotImplemented("enumerate_components"))?;

    struct State {
        out: Vec<ComponentDescriptor>,
    }

    unsafe extern "C" fn thunk(
        name: *const c_char,
        stable_hash: u64,
        fields_json: *const c_char,
        user: *mut core::ffi::c_void,
    ) {
        if name.is_null() || fields_json.is_null() || user.is_null() {
            return;
        }
        // SAFETY: host contract guarantees UTF-8 + null-term for this call.
        let n = unsafe { CStr::from_ptr(name) };
        let fj = unsafe { CStr::from_ptr(fields_json) };
        let (Ok(n), Ok(fj)) = (n.to_str(), fj.to_str()) else { return };
        // SAFETY: `user` is our boxed State.
        let state = unsafe { &mut *(user as *mut State) };
        state.out.push(ComponentDescriptor {
            name: n.to_owned(),
            stable_hash,
            fields_json: fj.to_owned(),
        });
    }

    let mut state = State { out: Vec::new() };
    let user = (&mut state) as *mut State as *mut core::ffi::c_void;
    // SAFETY: `thunk` matches the BldComponentEnumFn signature; `user` is
    // valid for the duration of this synchronous call.
    unsafe { f(thunk, user) };
    Ok(state.out)
}

/// Enqueue a single serialized command. Returns the command id.
pub fn run_command(opcode: u32, payload: &[u8]) -> Result<u64, HostError> {
    let h = host()?;
    let f = h.run_command.ok_or(HostError::NotImplemented("run_command"))?;
    // SAFETY: `payload.as_ptr()` is valid for `payload.len()` bytes; u32-cast ok because
    // payload is bounded; see run_command header contract.
    let id = unsafe { f(opcode, payload.as_ptr() as *const core::ffi::c_void, payload.len() as u32) };
    if id == 0 {
        Err(HostError::Failed("run_command returned 0"))
    } else {
        Ok(id)
    }
}

// =============================================================================
// Tests (pure-Rust; no host)
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use core::ffi::CStr;

    #[test]
    fn bridge_reexports_ffi() {
        // SAFETY: static, null-terminated UTF-8.
        let text = unsafe { CStr::from_ptr(bld_hello()) }
            .to_str()
            .expect("BLD greeting must be valid UTF-8");
        assert!(text.contains("BLD online"));
    }

    #[test]
    fn abi_version_is_one() {
        assert_eq!(bld_abi_version(), 1);
    }

    #[test]
    fn typed_wrappers_return_no_host_when_unwired() {
        // Ensure any leftover table from another test is cleared.
        unsafe { bld_abi_install_host_callbacks(core::ptr::null()) };
        assert!(matches!(log(0, "hi"), Err(HostError::NoHost)));
        assert!(matches!(create_entity(None), Err(HostError::NoHost)));
        assert!(matches!(destroy_entity(1), Err(HostError::NoHost)));
        assert!(matches!(undo(), Err(HostError::NoHost)));
        assert!(matches!(enumerate_components(), Err(HostError::NoHost)));
    }

    // --- in-memory fake host for testing the bridge itself. ---

    static mut LAST_LOG: [u8; 256] = [0; 256];
    static LAST_LOG_LEN: core::sync::atomic::AtomicUsize =
        core::sync::atomic::AtomicUsize::new(0);
    static LAST_ENT_NAME: core::sync::atomic::AtomicUsize =
        core::sync::atomic::AtomicUsize::new(0);

    unsafe extern "C" fn fake_log(_level: u32, msg: *const c_char) {
        if msg.is_null() {
            return;
        }
        // SAFETY: per-test only.
        let s = unsafe { CStr::from_ptr(msg) };
        let bytes = s.to_bytes();
        let len = bytes.len().min(255);
        // SAFETY: single-threaded test.
        unsafe {
            let addr = core::ptr::addr_of_mut!(LAST_LOG);
            (&mut *addr)[..len].copy_from_slice(&bytes[..len]);
        }
        LAST_LOG_LEN.store(len, core::sync::atomic::Ordering::SeqCst);
    }

    unsafe extern "C" fn fake_create_entity(name: *const c_char) -> u64 {
        if !name.is_null() {
            LAST_ENT_NAME.store(1, core::sync::atomic::Ordering::SeqCst);
        }
        42
    }

    #[test]
    fn fake_host_dispatches_log_and_create() {
        let mut table = BldHostCallbacks::empty();
        table.log = Some(fake_log);
        table.create_entity = Some(fake_create_entity);
        unsafe { bld_abi_install_host_callbacks(&table) };

        log(1, "hello bridge").expect("log should dispatch");
        let len = LAST_LOG_LEN.load(core::sync::atomic::Ordering::SeqCst);
        // SAFETY: single-threaded test read.
        let raw = unsafe {
            let addr = core::ptr::addr_of!(LAST_LOG);
            (&*addr)[..len].to_vec()
        };
        assert_eq!(std::str::from_utf8(&raw).unwrap(), "hello bridge");

        let handle = create_entity(Some("Cube")).expect("create_entity");
        assert_eq!(handle, 42);
        assert_eq!(
            LAST_ENT_NAME.load(core::sync::atomic::Ordering::SeqCst),
            1
        );

        // Clear.
        unsafe { bld_abi_install_host_callbacks(core::ptr::null()) };
    }
}
