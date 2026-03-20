#include "patch_archive.h"
#include "patch_log.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <windows.h>

std::vector<mz_zip_archive>                          PatchArchive::s_archives;
std::vector<std::string>                             PatchArchive::s_names;
std::unordered_map<std::string, PatchArchive::Entry> PatchArchive::s_index;

std::string PatchArchive::Normalize(const std::string& name) {
    size_t sep = name.find_last_of("/\\");
    std::string out = (sep != std::string::npos) ? name.substr(sep + 1) : name;
    for (char& c : out)
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return out;
}

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], len, nullptr, nullptr);
    return out;
}

static std::string BaseName(const std::wstring& path) {
    size_t sep = path.find_last_of(L"/\\");
    std::wstring name = sep != std::wstring::npos ? path.substr(sep + 1) : path;
    return WideToUtf8(name);
}

// Use FindFirstFileW/FindNextFileW so Japanese/Unicode directory names work.
static std::vector<std::wstring> FindFanpatches(const std::wstring& gameDir) {
    std::vector<std::wstring> results;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((gameDir + L"*.fanpatch").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return results;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            results.push_back(gameDir + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(results.begin(), results.end());
    return results;
}

void PatchArchive::LoadAll(const std::wstring& gameDir) {
    auto files = FindFanpatches(gameDir);
    if (files.empty()) return;

    s_archives.resize(files.size());
    s_names.resize(files.size());

    for (size_t i = 0; i < files.size(); ++i) {
        s_names[i] = BaseName(files[i]);

        // miniz needs a UTF-8 path on Windows for unicode paths
        std::string utf8path = WideToUtf8(files[i]);

        mz_zip_archive& zip = s_archives[i];
        memset(&zip, 0, sizeof(zip));
        if (!mz_zip_reader_init_file(&zip, utf8path.c_str(), 0)) continue;

        mz_uint count = mz_zip_reader_get_num_files(&zip);
        for (mz_uint j = 0; j < count; ++j) {
            if (mz_zip_reader_is_file_a_directory(&zip, j)) continue;
            char fname[512];
            mz_zip_reader_get_filename(&zip, j, fname, sizeof(fname));
            s_index[Normalize(fname)] = { i, j, s_names[i] };
        }
    }
}

bool PatchArchive::Has(const std::string& filename) {
    return s_index.count(Normalize(filename)) > 0;
}

std::vector<uint8_t> PatchArchive::Extract(const std::string& filename) {
    std::vector<uint8_t> result;
    auto it = s_index.find(Normalize(filename));
    if (it == s_index.end()) return result;

    PatchLogger::Log(it->first, it->second.sourceName);

    mz_zip_archive& zip = s_archives[it->second.archiveIndex];
    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&zip, it->second.fileIndex, &stat)) return result;

    result.resize(static_cast<size_t>(stat.m_uncomp_size));
    if (!mz_zip_reader_extract_to_mem(&zip, it->second.fileIndex,
                                       result.data(), result.size(), 0))
        result.clear();
    return result;
}

void PatchArchive::Unload() {
    for (auto& zip : s_archives) mz_zip_reader_end(&zip);
    s_archives.clear();
    s_names.clear();
    s_index.clear();
}
