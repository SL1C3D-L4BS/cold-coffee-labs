// editor/bld_bridge/mcp_stdio_client.cpp

#include "editor/bld_bridge/mcp_stdio_client.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace gw::editor::bld {

struct McpStdioSession::Impl {
#if defined(_WIN32)
    HANDLE child_process = INVALID_HANDLE_VALUE;
    HANDLE stdin_wr      = INVALID_HANDLE_VALUE;
    HANDLE stdout_rd     = INVALID_HANDLE_VALUE;
#else
    pid_t  child_pid = -1;
    int    stdin_wr  = -1;
    int    stdout_rd = -1;
#endif
    std::string read_accum;
};

std::string json_escape_string(std::string_view s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char uc : s) {
        const char c = static_cast<char>(uc);
        switch (c) {
        case '"':
            o += "\\\"";
            break;
        case '\\':
            o += "\\\\";
            break;
        case '\b':
            o += "\\b";
            break;
        case '\f':
            o += "\\f";
            break;
        case '\n':
            o += "\\n";
            break;
        case '\r':
            o += "\\r";
            break;
        case '\t':
            o += "\\t";
            break;
        default:
            if (uc < 0x20) {
                char buf[8];
                (void)std::snprintf(buf, sizeof buf, "\\u%04x", uc);
                o += buf;
            } else {
                o.push_back(c);
            }
        }
    }
    return o;
}

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

[[nodiscard]] bool write_all_impl(
#if defined(_WIN32)
    HANDLE h,
#else
    int h,
#endif
    const char* p,
    std::size_t   n,
    std::string&  err) {
#if defined(_WIN32)
    std::size_t off = 0;
    while (off < n) {
        DWORD w = 0;
        if (!WriteFile(h, p + off, static_cast<DWORD>(n - off), &w, nullptr) || w == 0) {
            err = "WriteFile failed writing to gw_bld_server stdin";
            return false;
        }
        off += static_cast<std::size_t>(w);
    }
#else
    std::size_t off = 0;
    while (off < n) {
        const ssize_t w = ::write(h, p + off, n - off);
        if (w <= 0) {
            err = "write failed to gw_bld_server stdin";
            return false;
        }
        off += static_cast<std::size_t>(w);
    }
#endif
    return true;
}

[[nodiscard]] bool read_chunk(
#if defined(_WIN32)
    HANDLE h,
#else
    int h,
#endif
    std::string& accum,
    std::string& err,
    int          timeout_ms) {
#if defined(_WIN32)
    DWORD avail = 0;
    if (!PeekNamedPipe(h, nullptr, 0, nullptr, &avail, nullptr)) {
        err = "PeekNamedPipe failed";
        return false;
    }
    if (avail == 0) {
        return true; // no data yet
    }
    std::vector<char> buf(static_cast<std::size_t>(avail));
    DWORD got = 0;
    if (!ReadFile(h, buf.data(), static_cast<DWORD>(buf.size()), &got, nullptr)) {
        err = "ReadFile failed";
        return false;
    }
    if (got > 0) {
        accum.append(buf.data(), static_cast<std::size_t>(got));
    }
    (void)timeout_ms;
#else
    pollfd pfd{};
    pfd.fd     = h;
    pfd.events = POLLIN;
    const int pr = poll(&pfd, 1, timeout_ms);
    if (pr < 0) {
        err = "poll failed on gw_bld_server stdout";
        return false;
    }
    if (pr == 0) {
        return true;
    }
    char buf[8192];
    const ssize_t n = read(h, buf, sizeof buf);
    if (n < 0) {
        err = "read failed on gw_bld_server stdout";
        return false;
    }
    if (n > 0) {
        accum.append(buf, static_cast<std::size_t>(n));
    }
#endif
    return true;
}

[[nodiscard]] bool extract_one_frame(std::string& accum, std::string& body_out, std::string& err) {
    const std::string delim = "\r\n\r\n";
    const auto        pos   = accum.find(delim);
    if (pos == std::string::npos) {
        if (accum.size() > 8 * 1024 * 1024) {
            err = "MCP header exceeds buffer guard";
            return false;
        }
        return false;
    }
    const std::string headers = accum.substr(0, pos);
    std::size_t       cl      = 0;
    bool              have_cl = false;
    {
        std::istringstream hs(headers);
        std::string        line;
        while (std::getline(hs, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }
            const auto colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            std::string key = line.substr(0, colon);
            for (char& c : key) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (key == "content-length") {
                std::string v = line.substr(colon + 1);
                while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) {
                    v.erase(v.begin());
                }
                cl     = static_cast<std::size_t>(std::strtoull(v.c_str(), nullptr, 10));
                have_cl = true;
            }
        }
    }
    if (!have_cl) {
        err = "missing Content-Length in MCP frame";
        return false;
    }
    const std::size_t body_off = pos + delim.size();
    if (accum.size() < body_off + cl) {
        return false;
    }
    body_out.assign(accum.data() + body_off, cl);
    accum.erase(0, body_off + cl);
    return true;
}

[[nodiscard]] bool write_frame_impl(
#if defined(_WIN32)
    HANDLE wr,
#else
    int wr,
#endif
    const std::string& body,
    std::string&       err) {
    std::ostringstream hdr;
    hdr << "Content-Length: " << body.size() << "\r\n\r\n";
    const std::string h = hdr.str();
    if (!write_all_impl(wr, h.data(), h.size(), err)) {
        return false;
    }
    if (!body.empty() && !write_all_impl(wr, body.data(), body.size(), err)) {
        return false;
    }
    return true;
}

}  // namespace

McpStdioSession::McpStdioSession() = default;

McpStdioSession::~McpStdioSession() {
    stop();
}

McpStdioSession::McpStdioSession(McpStdioSession&& o) noexcept
    : impl_(std::move(o.impl_)) {}

McpStdioSession& McpStdioSession::operator=(McpStdioSession&& o) noexcept {
    if (this != &o) {
        stop();
        impl_ = std::move(o.impl_);
    }
    return *this;
}

void McpStdioSession::stop() noexcept {
    if (!impl_) {
        return;
    }
#if defined(_WIN32)
    if (impl_->child_process != INVALID_HANDLE_VALUE) {
        (void)TerminateProcess(impl_->child_process, 1);
        (void)WaitForSingleObject(impl_->child_process, 5000);
        CloseHandle(impl_->child_process);
        impl_->child_process = INVALID_HANDLE_VALUE;
    }
    if (impl_->stdin_wr != INVALID_HANDLE_VALUE) {
        CloseHandle(impl_->stdin_wr);
        impl_->stdin_wr = INVALID_HANDLE_VALUE;
    }
    if (impl_->stdout_rd != INVALID_HANDLE_VALUE) {
        CloseHandle(impl_->stdout_rd);
        impl_->stdout_rd = INVALID_HANDLE_VALUE;
    }
#else
    if (impl_->child_pid > 0) {
        (void)kill(impl_->child_pid, SIGTERM);
        int st = 0;
        (void)waitpid(impl_->child_pid, &st, 0);
        impl_->child_pid = -1;
    }
    if (impl_->stdin_wr >= 0) {
        close(impl_->stdin_wr);
        impl_->stdin_wr = -1;
    }
    if (impl_->stdout_rd >= 0) {
        close(impl_->stdout_rd);
        impl_->stdout_rd = -1;
    }
#endif
    impl_.reset();
}

bool McpStdioSession::start(const std::string& server_exe,
                            const std::string& project_root_utf8,
                            std::string&       err_out) {
    stop();
    if (server_exe.empty()) {
        err_out = "empty GW_BLD_SERVER_EXE";
        return false;
    }

    auto impl = std::make_unique<Impl>();

#if defined(_WIN32)
    SECURITY_ATTRIBUTES sa{};
    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE out_rd = INVALID_HANDLE_VALUE;
    HANDLE out_wr = INVALID_HANDLE_VALUE;
    HANDLE in_rd  = INVALID_HANDLE_VALUE;
    HANDLE in_wr  = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&out_rd, &out_wr, &sa, 0)) {
        err_out = "CreatePipe stdout failed";
        return false;
    }
    if (!CreatePipe(&in_rd, &in_wr, &sa, 0)) {
        CloseHandle(out_rd);
        CloseHandle(out_wr);
        err_out = "CreatePipe stdin failed";
        return false;
    }
    if (!SetHandleInformation(out_rd, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(out_rd);
        CloseHandle(out_wr);
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        err_out = "SetHandleInformation stdout failed";
        return false;
    }
    if (!SetHandleInformation(in_wr, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(out_rd);
        CloseHandle(out_wr);
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        err_out = "SetHandleInformation stdin failed";
        return false;
    }

    STARTUPINFOA si{};
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdOutput = out_wr;
    si.hStdError  = out_wr;
    si.hStdInput  = in_rd;

    std::string cmdline = quote_win_arg(server_exe);
    cmdline += ' ';
    cmdline += quote_win_arg("--project-root");
    cmdline += ' ';
    cmdline += quote_win_arg(project_root_utf8);

    std::vector<char> cmd_mut(cmdline.begin(), cmdline.end());
    cmd_mut.push_back('\0');

    PROCESS_INFORMATION pi{};
    const BOOL ok = CreateProcessA(server_exe.c_str(), cmd_mut.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(out_wr);
    CloseHandle(in_rd);
    if (!ok) {
        CloseHandle(out_rd);
        CloseHandle(in_wr);
        err_out = "CreateProcess gw_bld_server failed";
        return false;
    }
    CloseHandle(pi.hThread);
    impl->child_process = pi.hProcess;
    impl->stdin_wr      = in_wr;
    impl->stdout_rd     = out_rd;
#else
    int p_out[2]{};
    int p_in[2]{};
    if (pipe(p_out) != 0 || pipe(p_in) != 0) {
        err_out = "pipe() failed";
        return false;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        close(p_out[0]);
        close(p_out[1]);
        close(p_in[0]);
        close(p_in[1]);
        err_out = "fork failed";
        return false;
    }
    if (pid == 0) {
        close(p_out[0]);
        close(p_in[1]);
        if (dup2(p_out[1], STDOUT_FILENO) < 0 || dup2(p_out[1], STDERR_FILENO) < 0 ||
            dup2(p_in[0], STDIN_FILENO) < 0) {
            _exit(127);
        }
        close(p_out[1]);
        close(p_in[0]);
        char arg_root[] = "--project-root";
        char* argv[]   = {const_cast<char*>(server_exe.c_str()), arg_root,
                        const_cast<char*>(project_root_utf8.c_str()), nullptr};
        execv(server_exe.c_str(), argv);
        _exit(127);
    }
    close(p_out[1]);
    close(p_in[0]);
    impl->child_pid  = pid;
    impl->stdout_rd = p_out[0];
    impl->stdin_wr  = p_in[1];
#endif

    impl_ = std::move(impl);

    // MCP handshake: initialize (id=1).
    std::ostringstream init;
    init << "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{"
            "\"protocolVersion\":\"2025-11-25\","
            "\"clientInfo\":{\"name\":\"gw_editor\",\"version\":\"1\"},"
            "\"capabilities\":{}}}";
    const std::string init_body = init.str();
    std::string       resp;
    if (!request(init_body, resp, err_out)) {
        stop();
        return false;
    }
    return true;
}

bool McpStdioSession::request(const std::string& request_body_utf8,
                             std::string&       response_body_out,
                             std::string&       err_out) {
    response_body_out.clear();
    err_out.clear();
    if (!impl_) {
        err_out = "MCP session not started";
        return false;
    }

#if defined(_WIN32)
    HANDLE wr = impl_->stdin_wr;
    HANDLE rd = impl_->stdout_rd;
#else
    int wr = impl_->stdin_wr;
    int rd = impl_->stdout_rd;
#endif

    if (!write_frame_impl(wr, request_body_utf8, err_out)) {
        return false;
    }

    impl_->read_accum.clear();
    for (int spin = 0; spin < 4000; ++spin) {
#if defined(_WIN32)
        if (WaitForSingleObject(impl_->child_process, 0) == WAIT_OBJECT_0) {
            err_out = "gw_bld_server exited before MCP response";
            return false;
        }
#else
        {
            int           st = 0;
            const pid_t w = waitpid(impl_->child_pid, &st, WNOHANG);
            if (w == impl_->child_pid) {
                err_out = "gw_bld_server exited before MCP response";
                return false;
            }
        }
#endif
        if (!read_chunk(rd, impl_->read_accum, err_out, 25)) {
            return false;
        }
        std::string body;
        std::string frame_err;
        if (extract_one_frame(impl_->read_accum, body, frame_err)) {
            response_body_out = std::move(body);
            return true;
        }
        if (!frame_err.empty()) {
            err_out = std::move(frame_err);
            return false;
        }
#if defined(_WIN32)
        Sleep(1);
#else
        (void)spin;
#endif
    }
    err_out = "timeout waiting for MCP framed response";
    return false;
}

}  // namespace gw::editor::bld
