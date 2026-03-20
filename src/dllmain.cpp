#include <windows.h>
#include <string>
#include "patch_archive.h"
#include "patch_log.h"
#include "hooks.h"

extern void VersionProxy_Init();

// Use wide APIs throughout to handle non-ASCII game directory paths
// (e.g. Japanese characters in the folder name).
static std::wstring GetGameDirW() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s(path);
    size_t slash = s.rfind(L'\\');
    return slash != std::wstring::npos ? s.substr(0, slash + 1) : std::wstring();
}

// Narrow conversion for patch log init (log path only, ASCII-safe suffix)
static std::string WideToAnsi(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(len - 1, '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &out[0], len, nullptr, nullptr);
    return out;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        VersionProxy_Init();
        {
            std::wstring gameDirW = GetGameDirW();
            PatchLogger::Init(WideToAnsi(gameDirW));     // log path only needs ANSI
            PatchArchive::LoadAll(gameDirW);         // wide path for ZIP scanning
            Hooks_Install();
        }
        break;
    case DLL_PROCESS_DETACH:
        Hooks_Uninstall();
        PatchArchive::Unload();
        PatchLogger::Shutdown();
        break;
    }
    return TRUE;
}
