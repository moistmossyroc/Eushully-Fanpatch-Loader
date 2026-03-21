#pragma once
#include <string>

// Logs each asset served from a .fanpatch archive to fanpatch_log.txt.
// Thread-safe.
class PatchLogger {
public:
    static void Init(const std::wstring& gameDir);
    static void Log(const std::string& filename, const std::string& source);
    static void Shutdown();
};
