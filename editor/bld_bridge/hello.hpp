#pragma once

// editor/bld_bridge/hello — the narrow C++ facade over bld-ffi's Phase-1
// symbol surface. Phase 9 replaces this with the full cxx/cbindgen bridge
// generated alongside bld-bridge. See docs/01_CONSTITUTION_AND_PROGRAM.md §2.5, docs/02_ROADMAP_AND_OPERATIONS.md §13.

namespace gw::editor {

// Returns the null-terminated, UTF-8 greeting produced by the Rust BLD crate.
// Thread-safe; returns a pointer into Rust-side static memory (never null under
// normal operation). Do not free.
[[nodiscard]] const char* bld_greeting() noexcept;

}  // namespace gw::editor
