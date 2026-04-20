//! bld-ffi — the stable C-ABI surface exposed by BLD to the C++ engine.
//!
//! Every symbol in this crate is `extern "C"` and uses only `#[repr(C)]` types.
//! No generics, no trait objects, no Rust-native error types cross this boundary.
//!
//! Miri runs nightly on this crate to catch UB at the boundary.
//! See docs/00 §2.5 and non-negotiable #15.

#![deny(unsafe_op_in_unsafe_fn)]
#![warn(missing_docs)]

use core::ffi::c_char;

/// The Phase-1 *First Light* BLD greeting. Null-terminated, UTF-8.
/// Stored in `.rodata`; lifetime is `'static`.
const BLD_GREETING: &[u8] = b"BLD online. Brewed slowly. Built deliberately.\0";

/// Returns a pointer to a static, null-terminated UTF-8 greeting.
///
/// # Safety
/// The returned pointer is `'static` and must not be freed by the caller. It
/// remains valid for the entire process lifetime.
#[unsafe(no_mangle)]
pub extern "C" fn bld_hello() -> *const c_char {
    BLD_GREETING.as_ptr() as *const c_char
}

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
}
