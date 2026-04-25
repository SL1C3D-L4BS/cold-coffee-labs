#include "engine/platform/process.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cstring>

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

#if defined(_WIN32)
namespace {

[[nodiscard]] std::string quote_win_arg(const std::string& a) {
    std::string o;
    o.reserve(a.size() + 2);
    o.push_back('"');
    for (char c : a) {
        if (c == '"') {
            o.push_back('\\');
        }
        o.push_back(c);
    }
    o.push_back('"');
    return o;
}

} // namespace
#endif

RunCommandCaptureResult run_command_capture(const std::string&              executable,
                                           const std::vector<std::string>& args,
                                           const std::atomic<bool>*        cancel_requested) {
    RunCommandCaptureResult r;
    if (executable.empty()) {
        return r;
    }

#if defined(_WIN32)
    SECURITY_ATTRIBUTES sa{};
    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE rd = INVALID_HANDLE_VALUE;
    HANDLE wr = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        return r;
    }
    if (!SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(rd);
        CloseHandle(wr);
        return r;
    }

    STARTUPINFOA si{};
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = wr;
    si.hStdError  = wr;

    std::string cmdline = quote_win_arg(executable);
    for (const auto& a : args) {
        cmdline.push_back(' ');
        cmdline += quote_win_arg(a);
    }
    std::vector<char> cmd_mut(cmdline.begin(), cmdline.end());
    cmd_mut.push_back('\0');

    PROCESS_INFORMATION pi{};
    const BOOL ok = CreateProcessA(executable.c_str(), cmd_mut.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(wr);
    wr = INVALID_HANDLE_VALUE;
    if (!ok) {
        CloseHandle(rd);
        return r;
    }
    CloseHandle(pi.hThread);
    r.started = true;

    for (;;) {
        if (cancel_requested != nullptr && cancel_requested->load(std::memory_order_relaxed)) {
            TerminateProcess(pi.hProcess, 130);
            break;
        }
        DWORD avail = 0;
        if (PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
            std::vector<char> buf(static_cast<std::size_t>(avail) + 1u);
            DWORD             got = 0;
            if (ReadFile(rd, buf.data(), avail, &got, nullptr) && got > 0) {
                r.combined_output.append(buf.data(), got);
            }
            continue;
        }
        const DWORD w = WaitForSingleObject(pi.hProcess, 25);
        if (w == WAIT_OBJECT_0) {
            break;
        }
    }

    (void)WaitForSingleObject(pi.hProcess, 10000);
    DWORD code = static_cast<DWORD>(-1);
    (void)GetExitCodeProcess(pi.hProcess, &code);
    r.exit_code = static_cast<int>(code);

    for (;;) {
        char    buf[4096];
        DWORD   got = 0;
        const BOOL rr = ReadFile(rd, buf, sizeof buf, &got, nullptr);
        if (!rr || got == 0) {
            break;
        }
        r.combined_output.append(buf, got);
    }

    CloseHandle(rd);
    CloseHandle(pi.hProcess);
    return r;

#else
    int pipefd[2]{};
    if (pipe(pipefd) != 0) {
        return r;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return r;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(pipefd[1]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);
        execvp(executable.c_str(), argv.data());
        _exit(127);
    }

    close(pipefd[1]);
    (void)fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    r.started = true;

    for (;;) {
        if (cancel_requested != nullptr && cancel_requested->load(std::memory_order_relaxed)) {
            (void)kill(pid, SIGKILL);
            int st = 0;
            (void)waitpid(pid, &st, 0);
            r.exit_code = 130;
            break;
        }

        pollfd pfd{};
        pfd.fd     = pipefd[0];
        pfd.events = POLLIN;
        const int pr = poll(&pfd, 1, 25);
        if (pr > 0 && (pfd.revents & POLLIN) != 0) {
            char    buf[4096];
            const ssize_t n = read(pipefd[0], buf, sizeof buf);
            if (n > 0) {
                r.combined_output.append(buf, static_cast<std::size_t>(n));
            }
        }

        int   st   = 0;
        const pid_t w = waitpid(pid, &st, WNOHANG);
        if (w == pid) {
            if (WIFEXITED(st)) {
                r.exit_code = WEXITSTATUS(st);
            } else if (WIFSIGNALED(st)) {
                r.exit_code = 128 + WTERMSIG(st);
            } else {
                r.exit_code = -1;
            }
            break;
        }
    }

    for (;;) {
        char          buf[4096];
        const ssize_t n = read(pipefd[0], buf, sizeof buf);
        if (n <= 0) {
            break;
        }
        r.combined_output.append(buf, static_cast<std::size_t>(n));
    }
    close(pipefd[0]);
    return r;
#endif
}

} // namespace gw::platform
