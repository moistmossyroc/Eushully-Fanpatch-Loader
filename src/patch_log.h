#pragma once
#include <string>

// Logs patch activity to fanpatch_log.txt in the game directory.
// Records each asset served from a .fanpatch archive so you can verify
// which files are being patched and which archive they came from.
// Thread-safe. Opens on Init() and closes cleanly on Shutdown().
class PatchLogger {
public:
    static void Init(const std::string& gameDir);
    static void Log(const std::string& filename, const std::string& source);
    static void Shutdown();
};
