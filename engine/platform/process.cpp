#include "engine/platform/process.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace gw::platform {

std::string current_executable_path() {
#if defined(_WIN32)
    char buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return {};
    return std::string(buf, buf + n);
#else
    char    buf[4096]{};
    ssize_t r = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (r <= 0) return {};
    return std::string(buf, static_cast<std::size_t>(r));
#endif
}

bool spawn_detached(const std::string& executable, const std::vector<std::string>& args) {
    if (executable.empty()) return false;

#if defined(_WIN32)
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::string cmd = "\"";
    cmd += executable;
    cmd += '"';
    for (const auto& a : args) {
        cmd += ' ';
        cmd += a;
    }
    std::vector<char> cmdline(cmd.begin(), cmd.end());
    cmdline.push_back('\0');

    const BOOL ok = CreateProcessA(nullptr, cmdline.data(), nullptr, nullptr, FALSE,
                                   CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
                                   nullptr, nullptr, &si, &pi);
    if (!ok) return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    std::vector<std::string> all;
    all.reserve(1 + args.size());
    all.push_back(executable);
    for (const auto& a : args) all.push_back(a);
    std::vector<char*> argv;
    for (auto& s : all) argv.push_back(s.data());
    argv.push_back(nullptr);

    const pid_t intermediate = fork();
    if (intermediate < 0) return false;
    if (intermediate > 0) {
        (void)waitpid(intermediate, nullptr, 0);
        return true;
    }
    if (setsid() < 0) _exit(127);
    const pid_t worker = fork();
    if (worker < 0) _exit(127);
    if (worker > 0) _exit(0);

    execv(all[0].c_str(), argv.data());
    _exit(127);
#endif
}

} // namespace gw::platform
