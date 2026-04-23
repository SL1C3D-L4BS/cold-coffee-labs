// editor/shell/file_dialog.cpp

#include "editor/shell/file_dialog.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#include <combaseapi.h>
#else
#include <cstdio>
#include <string>
#endif

namespace gw::editor::shell {

std::optional<std::filesystem::path> pick_folder() noexcept {
#ifdef _WIN32
    std::optional<std::filesystem::path> result;
    HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool co_ok = SUCCEEDED(co) || co == RPC_E_CHANGED_MODE;
    if (!co_ok) return std::nullopt;

    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(&dlg)))) {
        if (SUCCEEDED(co)) CoUninitialize();
        return std::nullopt;
    }

    DWORD opts = 0;
    if (SUCCEEDED(dlg->GetOptions(&opts))) {
        dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    }

    if (FAILED(dlg->Show(nullptr))) {
        dlg->Release();
        if (SUCCEEDED(co)) CoUninitialize();
        return std::nullopt;
    }

    IShellItem* item = nullptr;
    if (SUCCEEDED(dlg->GetResult(&item))) {
        PWSTR path_w = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_w))) {
            char path_a[MAX_PATH * 4]{};
            WideCharToMultiByte(CP_UTF8, 0, path_w, -1, path_a,
                                static_cast<int>(sizeof(path_a)), nullptr, nullptr);
            result = std::filesystem::path{path_a};
            CoTaskMemFree(path_w);
        }
        item->Release();
    }
    dlg->Release();
    if (SUCCEEDED(co)) CoUninitialize();
    return result;
#else
    FILE* p = popen("zenity --file-selection --directory 2>/dev/null", "r");
    if (!p) return std::nullopt;
    char buf[4096]{};
    if (!fgets(buf, sizeof(buf), p)) {
        pclose(p);
        return std::nullopt;
    }
    pclose(p);
    std::string s = buf;
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    if (s.empty()) return std::nullopt;
    return std::filesystem::path{s};
#endif
}

} // namespace gw::editor::shell
