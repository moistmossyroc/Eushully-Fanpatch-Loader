#include "vfile.h"

std::unordered_map<HANDLE, VFileTable::VFile> VFileTable::s_table;
std::mutex                                     VFileTable::s_mutex;

HANDLE VFileTable::Create(std::vector<uint8_t> data) {
    // Non-signaled manual-reset event — cheapest valid unique HANDLE.
    HANDLE h = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!h || h == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
    std::lock_guard<std::mutex> lk(s_mutex);
    s_table.emplace(h, VFile{ std::move(data), 0u });
    return h;
}

bool VFileTable::Owns(HANDLE h) {
    std::lock_guard<std::mutex> lk(s_mutex);
    return s_table.count(h) > 0;
}

bool VFileTable::Read(HANDLE h, LPVOID buf, DWORD n, LPDWORD nRead) {
    std::lock_guard<std::mutex> lk(s_mutex);
    auto it = s_table.find(h);
    if (it == s_table.end()) return false;

    VFile& vf    = it->second;
    DWORD  avail = static_cast<DWORD>(vf.data.size()) - vf.pos;
    DWORD  got   = n < avail ? n : avail;
    if (got) memcpy(buf, vf.data.data() + vf.pos, got);
    vf.pos += got;
    if (nRead) *nRead = got;
    return true; // TRUE + nRead==0 is the standard EOF signal
}

DWORD VFileTable::Seek(HANDLE h, LONG dist, DWORD method) {
    std::lock_guard<std::mutex> lk(s_mutex);
    auto it = s_table.find(h);
    if (it == s_table.end()) return INVALID_SET_FILE_POINTER;

    VFile& vf   = it->second;
    DWORD  size = static_cast<DWORD>(vf.data.size());
    DWORD  newPos;
    switch (method) {
        case FILE_BEGIN:   newPos = static_cast<DWORD>(dist); break;
        case FILE_CURRENT: newPos = vf.pos + static_cast<DWORD>(dist); break;
        case FILE_END:     newPos = size   + static_cast<DWORD>(dist); break;
        default:           return INVALID_SET_FILE_POINTER;
    }
    if (newPos > size) newPos = size;
    vf.pos = newPos;
    return newPos;
}

DWORD VFileTable::Size(HANDLE h) {
    std::lock_guard<std::mutex> lk(s_mutex);
    auto it = s_table.find(h);
    if (it == s_table.end()) return INVALID_FILE_SIZE;
    return static_cast<DWORD>(it->second.data.size());
}

bool VFileTable::Close(HANDLE h) {
    std::lock_guard<std::mutex> lk(s_mutex);
    auto it = s_table.find(h);
    if (it == s_table.end()) return false;
    s_table.erase(it);
    return true;
}
