# Greywater

C++23 and Vulkan engine with an editor and supporting tools. Configure and build with CMake using the presets in `CMakePresets.json`.

Typical local flow (Windows `dev-win` preset):

```powershell
cmake --preset dev-win
cmake --build --preset dev-win
ctest --preset dev-win
```

Linux uses `dev-linux` in place of `dev-win`. Use a Clang-based toolchain; presets mirror what CI expects.
