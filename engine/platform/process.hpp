#pragma once
// engine/platform/process.hpp — detached child process spawn (editor “play in runtime”).

#include <string>
#include <vector>

namespace gw::platform {

/// Absolute path to the current executable (narrow / UTF-8 on Windows best-effort).
[[nodiscard]] std::string current_executable_path();

/// Spawn `executable` detached from the editor; `args` are additional argv tokens (not argv[0]).
[[nodiscard]] bool spawn_detached(const std::string& executable,
                                  const std::vector<std::string>& args);

} // namespace gw::platform
