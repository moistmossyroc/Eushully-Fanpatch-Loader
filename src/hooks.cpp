#include "patch_archive.h"
#include "vfile.h"
#include "MinHook.h"
#include <windows.h>
#include <string>
#include <cctype>

// ---------------------------------------------------------------------------
// Typedefs
// ---------------------------------------------------------------------------
using PFN_CreateFileA    = HANDLE(WINAPI*)(LPCSTR,  DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
using PFN_CreateFileW    = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
using PFN_ReadFile       = BOOL  (WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
using PFN_SetFilePointer = DWORD (WINAPI*)(HANDLE, LONG, PLONG, DWORD);
using PFN_GetFileSize    = DWORD (WINAPI*)(HANDLE, LPDWORD);
using PFN_CloseHandle    = BOOL  (WINAPI*)(HANDLE);

static PFN_CreateFileA    Real_CreateFileA    = nullptr;
static PFN_CreateFileW    Real_CreateFileW    = nullptr;
static PFN_ReadFile       Real_ReadFile       = nullptr;
static PFN_SetFilePointer Real_SetFilePointer = nullptr;
static PFN_GetFileSize    Real_GetFileSize    = nullptr;
static PFN_CloseHandle    Real_CloseHandle    = nullptr;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string NormalizeA(const char* path) {
    std::string s(path ? path : "");
    size_t sep = s.find_last_of("/\\");
    std::string name = sep != std::string::npos ? s.substr(sep + 1) : s;
    for (char& c : name)
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return name;
}

static std::string NormalizeW(const wchar_t* path) {
    if (!path) return {};
    const wchar_t* sep = wcsrchr(path, L'\\');
    if (!sep) sep = wcsrchr(path, L'/');
    const wchar_t* start = sep ? sep + 1 : path;

    // Convert to UTF-8 — avoids truncating non-ASCII characters with a narrow cast.
    int len = WideCharToMultiByte(CP_UTF8, 0, start, -1, nullptr, 0, nullptr, nullptr);
    std::string name(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, start, -1, &name[0], len, nullptr, nullptr);

    for (char& c : name)
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return name;
}

// Returns a virtual handle if filename is in a fanpatch, INVALID_HANDLE_VALUE otherwise.
static HANDLE TryPatch(const std::string& name) {
    if (PatchArchive::IsBlacklisted(name)) return INVALID_HANDLE_VALUE; // never serve blacklisted extensions
    if (!PatchArchive::Has(name)) return INVALID_HANDLE_VALUE;
    auto data = PatchArchive::Extract(name);
    if (data.empty()) return INVALID_HANDLE_VALUE;
    return VFileTable::Create(std::move(data));
}

// ---------------------------------------------------------------------------
// Hooks
// ---------------------------------------------------------------------------
HANDLE WINAPI Hook_CreateFileA(
    LPCSTR lpFileName, DWORD dwAccess, DWORD dwShare,
    LPSECURITY_ATTRIBUTES lpSA, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate)
{
    if (lpFileName && (dwAccess & GENERIC_READ) && dwDisp == OPEN_EXISTING) {
        HANDLE h = TryPatch(NormalizeA(lpFileName));
        if (h != INVALID_HANDLE_VALUE) return h;
    }
    return Real_CreateFileA(lpFileName, dwAccess, dwShare, lpSA, dwDisp, dwFlags, hTemplate);
}

HANDLE WINAPI Hook_CreateFileW(
    LPCWSTR lpFileName, DWORD dwAccess, DWORD dwShare,
    LPSECURITY_ATTRIBUTES lpSA, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate)
{
    if (lpFileName && (dwAccess & GENERIC_READ) && dwDisp == OPEN_EXISTING) {
        HANDLE h = TryPatch(NormalizeW(lpFileName));
        if (h != INVALID_HANDLE_VALUE) return h;
    }
    return Real_CreateFileW(lpFileName, dwAccess, dwShare, lpSA, dwDisp, dwFlags, hTemplate);
}

BOOL WINAPI Hook_ReadFile(
    HANDLE hFile, LPVOID lpBuf, DWORD nBytes, LPDWORD nRead, LPOVERLAPPED lpOv)
{
    if (VFileTable::Read(hFile, lpBuf, nBytes, nRead)) return TRUE;
    return Real_ReadFile(hFile, lpBuf, nBytes, nRead, lpOv);
}

DWORD WINAPI Hook_SetFilePointer(HANDLE hFile, LONG lDist, PLONG lpDistHigh, DWORD dwMethod) {
    DWORD pos = VFileTable::Seek(hFile, lDist, dwMethod);
    if (pos != INVALID_SET_FILE_POINTER) return pos;
    return Real_SetFilePointer(hFile, lDist, lpDistHigh, dwMethod);
}

DWORD WINAPI Hook_GetFileSize(HANDLE hFile, LPDWORD lpHigh) {
    DWORD sz = VFileTable::Size(hFile);
    if (sz != INVALID_FILE_SIZE) {
        if (lpHigh) *lpHigh = 0;
        return sz;
    }
    return Real_GetFileSize(hFile, lpHigh);
}

BOOL WINAPI Hook_CloseHandle(HANDLE hObject) {
    VFileTable::Close(hObject); // no-op if not ours
    return Real_CloseHandle(hObject);
}

// ---------------------------------------------------------------------------
// Install / Uninstall
// ---------------------------------------------------------------------------
bool Hooks_Install() {
    if (MH_Initialize() != MH_OK) return false;

    struct { const wchar_t* mod; const char* fn; LPVOID det; LPVOID* orig; } hooks[] = {
        { L"kernel32", "CreateFileA",    (LPVOID)Hook_CreateFileA,    (LPVOID*)&Real_CreateFileA    },
        { L"kernel32", "CreateFileW",    (LPVOID)Hook_CreateFileW,    (LPVOID*)&Real_CreateFileW    },
        { L"kernel32", "ReadFile",       (LPVOID)Hook_ReadFile,       (LPVOID*)&Real_ReadFile       },
        { L"kernel32", "SetFilePointer", (LPVOID)Hook_SetFilePointer, (LPVOID*)&Real_SetFilePointer },
        { L"kernel32", "GetFileSize",    (LPVOID)Hook_GetFileSize,    (LPVOID*)&Real_GetFileSize    },
        { L"kernel32", "CloseHandle",    (LPVOID)Hook_CloseHandle,    (LPVOID*)&Real_CloseHandle    },
    };

    for (auto& e : hooks)
        if (MH_CreateHookApi(e.mod, e.fn, e.det, e.orig) != MH_OK) return false;

    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

void Hooks_Uninstall() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
