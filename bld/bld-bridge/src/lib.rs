//! bld-bridge — the Rust-side of the C++ <-> Rust boundary.
//! Phase 9 lands the real cxx surface + cbindgen header. Phase 1 re-exports
//! bld-ffi so downstream callers see a stable crate name.

#![warn(missing_docs)]

pub use bld_ffi::bld_hello;

#[cfg(test)]
mod tests {
    use super::*;
    use core::ffi::CStr;

    #[test]
    fn bridge_reexports_ffi() {
        // SAFETY: `bld_hello` returns a static null-terminated C string.
        let text = unsafe { CStr::from_ptr(bld_hello()) }
            .to_str()
            .expect("BLD greeting must be valid UTF-8");
        assert!(text.contains("BLD online"));
    }
}
