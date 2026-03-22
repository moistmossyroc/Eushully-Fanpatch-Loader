#pragma once
#include <windows.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

// Backs a Win32 event HANDLE with an in-memory buffer to serve patched assets
// without writing to disk. Supports reads, seeks, and size queries.
class VFileTable {
public:
    // Create a virtual handle wrapping the given data.
    static HANDLE   Create(std::vector<uint8_t> data);

    // Returns true if h is one of ours.
    static bool     Owns(HANDLE h);

    // Implement ReadFile semantics. Returns false if h is not ours.
    static bool     Read(HANDLE h, LPVOID buf, DWORD n, LPDWORD nRead);

    // Implement SetFilePointer semantics. Returns INVALID_SET_FILE_POINTER if not ours.
    static DWORD    Seek(HANDLE h, LONG dist, DWORD method);

    // Implement GetFileSize. Returns INVALID_FILE_SIZE if not ours.
    static DWORD    Size(HANDLE h);

    // Frees the entry. Returns true if h was ours.
    static bool     Close(HANDLE h);

private:
    struct VFile {
        std::vector<uint8_t> data;
        DWORD                pos = 0;
    };
    static std::unordered_map<HANDLE, VFile> s_table;
    static std::mutex                        s_mutex;
};
