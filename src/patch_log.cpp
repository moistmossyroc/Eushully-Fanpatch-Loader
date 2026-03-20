#include "patch_log.h"
#include <windows.h>
#include <cstdio>
#include <mutex>

static FILE*      s_file  = nullptr;
static std::mutex s_mutex;

// Returns current time as a short string "HH:MM:SS"
static void WriteTimestamp(FILE* f) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
}

void PatchLogger::Init(const std::string& gameDir) {
    std::lock_guard<std::mutex> lk(s_mutex);
    std::string path = gameDir + "fanpatch_log.txt";
    fopen_s(&s_file, path.c_str(), "w");
    if (!s_file) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(s_file,
        "fanpatch activity log — %04u-%02u-%02u %02u:%02u:%02u\n"
        "------------------------------------------------------\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    fflush(s_file);
}

void PatchLogger::Log(const std::string& filename, const std::string& source) {
    std::lock_guard<std::mutex> lk(s_mutex);
    if (!s_file) return;
    WriteTimestamp(s_file);
    fprintf(s_file, "PATCH  %-40s  <- %s\n", filename.c_str(), source.c_str());
    fflush(s_file); // flush every line so the log is readable mid-session
}

void PatchLogger::Shutdown() {
    std::lock_guard<std::mutex> lk(s_mutex);
    if (s_file) {
        fprintf(s_file, "------------------------------------------------------\n");
        fprintf(s_file, "Session ended.\n");
        fclose(s_file);
        s_file = nullptr;
    }
}
