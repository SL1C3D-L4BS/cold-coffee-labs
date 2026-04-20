#include "editor/bld_bridge/hello.hpp"

// Declared by bld-ffi / bld-bridge (Rust, #[unsafe(no_mangle)] extern "C").
// See bld/bld-ffi/src/lib.rs.
extern "C" const char* bld_hello() noexcept;

namespace gw::editor {

const char* bld_greeting() noexcept {
    return bld_hello();
}

}  // namespace gw::editor
