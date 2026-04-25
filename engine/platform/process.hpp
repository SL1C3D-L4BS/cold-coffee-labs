#pragma once
// engine/platform/process.hpp — detached child process spawn (editor “play in runtime”).

#include <atomic>
#include <string>
#include <vector>

namespace gw::platform {

/// Absolute path to the current executable (narrow / UTF-8 on Windows best-effort).
[[nodiscard]] std::string current_executable_path();

/// Spawn `executable` detached from the editor; `args` are additional argv tokens (not argv[0]).
[[nodiscard]] bool spawn_detached(const std::string& executable,
                                  const std::vector<std::string>& args);

// ---------------------------------------------------------------------------
// Captured child process — stdout+stderr merged, optional cooperative cancel.
// When `cancel_requested` flips to true, the child is terminated (SIGKILL on
// POSIX, TerminateProcess on Windows) and `exit_code` is set to 130.
// ---------------------------------------------------------------------------
struct RunCommandCaptureResult {
    int         exit_code   = -1;
    std::string combined_output;
    bool        started     = false;
};

[[nodiscard]] RunCommandCaptureResult run_command_capture(
    const std::string&              executable,
    const std::vector<std::string>& args,
    const std::atomic<bool>*        cancel_requested = nullptr);

} // namespace gw::platform
