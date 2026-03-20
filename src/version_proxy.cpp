// version_proxy.cpp
// Forwards all version.dll exports to the real system DLL.
// Deliberately does NOT include <windows.h> to avoid winver.h conflicts.

extern "C" {
    unsigned long __stdcall GetSystemDirectoryA(char* buf, unsigned long size);
    void* __stdcall LoadLibraryA(const char* name);
    void* __stdcall GetProcAddress(void* hModule, const char* name);
}

static void* s_hReal = nullptr;

static void* LoadRealVersion() {
    char sys[260];
    GetSystemDirectoryA(sys, 260);
    char path[280];
    char* p = path;
    for (const char* s = sys; *s; ) *p++ = *s++;
    const char* suf = "\\version.dll";
    for (const char* s = suf; *s; ) *p++ = *s++;
    *p = '\0';
    return LoadLibraryA(path);
}

#define DECL(name)                                                           \
    static void* pfn_##name = nullptr;                                       \
    extern "C" __declspec(dllexport) __declspec(naked) void name() {         \
        __asm { jmp [pfn_##name] }                                           \
    }

DECL(GetFileVersionInfoA)
DECL(GetFileVersionInfoW)
DECL(GetFileVersionInfoExA)
DECL(GetFileVersionInfoExW)
DECL(GetFileVersionInfoSizeA)
DECL(GetFileVersionInfoSizeW)
DECL(GetFileVersionInfoSizeExA)
DECL(GetFileVersionInfoSizeExW)
DECL(VerFindFileA)
DECL(VerFindFileW)
DECL(VerInstallFileA)
DECL(VerInstallFileW)
DECL(VerLanguageNameA)
DECL(VerLanguageNameW)
DECL(VerQueryValueA)
DECL(VerQueryValueW)

#undef DECL

void VersionProxy_Init() {
    s_hReal = LoadRealVersion();
    if (!s_hReal) return;
    void* h = s_hReal;
#define LOAD(name) pfn_##name = GetProcAddress(h, #name)
    LOAD(GetFileVersionInfoA);        LOAD(GetFileVersionInfoW);
    LOAD(GetFileVersionInfoExA);      LOAD(GetFileVersionInfoExW);
    LOAD(GetFileVersionInfoSizeA);    LOAD(GetFileVersionInfoSizeW);
    LOAD(GetFileVersionInfoSizeExA);  LOAD(GetFileVersionInfoSizeExW);
    LOAD(VerFindFileA);               LOAD(VerFindFileW);
    LOAD(VerInstallFileA);            LOAD(VerInstallFileW);
    LOAD(VerLanguageNameA);           LOAD(VerLanguageNameW);
    LOAD(VerQueryValueA);             LOAD(VerQueryValueW);
#undef LOAD
}
