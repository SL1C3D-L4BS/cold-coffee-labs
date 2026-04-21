# BUILDING — Greywater_Engine

**Audience:** anyone who has just cloned the repo and wants a working build.
**Status:** Operational · Phase 1 minimum. Phase 2+ adds per-subsystem detail.

---

## Prerequisites

| Requirement | Minimum | Install |
| --- | --- | --- |
| Clang / LLVM | **18+** (we use 18–22) | `apt install clang-18 lld-18` (Linux) · LLVM installer or Chocolatey `choco install llvm` (Windows) |
| CMake | **3.28+** | https://cmake.org/download/ |
| Ninja | **1.11+** | `apt install ninja-build` · https://github.com/ninja-build/ninja/releases |
| Rust (stable) | **1.80+** | https://rustup.rs/ |
| Python | **3.10+** | (for `docs/07_DAILY_TODO_GENERATOR.py`) |
| Vulkan SDK | **1.4.341.1** (CI-pinned; engine targets 1.2 core + opportunistic 1.3) | https://vulkan.lunarg.com/sdk/home |
| Git | any | |

On Windows we use **clang-cl** (LLVM's MSVC-compatible driver). MSVC and GCC are not supported (`docs/00` §2.2).

Optional: `sccache` — pass `-DGW_USE_SCCACHE=ON` at configure time to wire it as the compiler launcher.

---

## First build

```bash
# Linux
cmake --preset dev-linux
cmake --build --preset dev-linux
ctest --preset dev-linux

# Windows (developer PowerShell or Developer Command Prompt is NOT required —
# clang-cl ships independently of Visual Studio)
cmake --preset dev-win
cmake --build --preset dev-win
ctest --preset dev-win
```

The first configure downloads CPM-managed dependencies (GLFW, volk, VMA, doctest, Corrosion) into `build/<preset>/_deps/`. Subsequent configures are offline.

Run the Phase-1 *First Light* binaries:

```bash
./build/dev-linux/bin/gw_sandbox   # hello-triangle
./build/dev-linux/bin/gw_editor    # prints the BLD greeting over the C-ABI
./build/dev-linux/bin/gw_tests     # doctest smoke suite
```

On Windows: `.\build\dev-win\bin\gw_sandbox.exe`, etc.

---

## Available presets

| Preset | Config | OS | Purpose |
| --- | --- | --- | --- |
| `dev-linux` | Debug | Linux | Default developer workflow |
| `dev-win` | Debug | Windows | Default developer workflow |
| `release-linux` | Release | Linux | Release build |
| `release-win` | Release | Windows | Release build |
| `linux-asan` | Debug + ASan | Linux | Nightly AddressSanitizer |
| `linux-ubsan` | Debug + UBSan | Linux | Nightly UndefinedBehaviorSanitizer |
| `linux-tsan` | Debug + TSan | Linux | Nightly ThreadSanitizer |

Workflow presets exist for `dev-linux` and `dev-win` — `cmake --workflow --preset dev-linux` does configure + build + test in one invocation.

---

## Rust side (BLD workspace)

Cargo reads `rust-toolchain.toml` and `bld/Cargo.toml` automatically. CMake drives Cargo through Corrosion as part of the main build; call Cargo directly only for tighter iteration:

```bash
cd bld
cargo build --release
cargo test
cargo clippy --workspace --all-targets --release -- -D warnings
cargo fmt --all
cargo +nightly miri test -p bld-ffi   # UB check on the FFI boundary (nightly)
```

`bld-ffi` is produced as both a `staticlib` and a `cdylib`; the editor and sandbox binaries link against the static archive.

---

## Troubleshooting

**`glslangValidator not found`** — install the Vulkan SDK or set `VULKAN_SDK` to its root directory. The sandbox compiles shaders at build time.

**`Corrosion not found` / Rust build errors** — ensure `rustup` / `cargo` are on PATH. Corrosion is fetched via CPM on first configure.

**Linker errors against Rust static library on Windows** — the editor's CMakeLists links `ws2_32`, `userenv`, `ntdll`, `bcrypt`. If you add new Rust crates that require more platform libraries, add them there.

**Validation layer warnings** — Debug builds enable `VK_LAYER_KHRONOS_validation`. Install the SDK's Vulkan-Loader component or set `VK_LAYER_PATH` to the SDK's layer directory.

**Line-ending warnings** — `.gitattributes` normalizes to LF. If you see warnings on Windows, run `git add --renormalize .` once.

---

## What's *not* here

Everything beyond the Phase 1 smoke. Run `git log` for landed work. The phase-gate milestone (`First Light`, week 004 — `docs/05_ROADMAP_AND_MILESTONES.md`) demands a two-minute recorded demo of the sandbox triangle + editor greeting on both Windows and Linux; that is stored under `docs/milestones/` when recorded.

---

*Built deliberately. Clang-only. Ninja-only. Windows + Linux equal. Shipped by Cold Coffee Labs.*
