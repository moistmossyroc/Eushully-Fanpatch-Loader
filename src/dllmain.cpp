#include <windows.h>
#include <string>
#include "patch_archive.h"
#include "patch_log.h"
#include "hooks.h"

extern void VersionProxy_Init();

// Returns the game directory using wide APIs to support non-ASCII paths.
static std::wstring GetGameDirW() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s(path);
    size_t slash = s.rfind(L'\\');
    return slash != std::wstring::npos ? s.substr(0, slash + 1) : std::wstring();
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        VersionProxy_Init();
        {
            std::wstring gameDirW = GetGameDirW();
            PatchLogger::Init(gameDirW);
            PatchArchive::LoadAll(gameDirW);
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
