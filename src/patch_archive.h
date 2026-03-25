#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "miniz.h"

// Scans the game directory for all *.fanpatch files (renamed ZIPs),
// loads them in alphabetical order. On filename collision the last
// archive alphabetically wins — prefix with 01_, 02_ etc. to control
// priority explicitly.
//
// Takes a wide game directory path to support non-ASCII folder names
// (e.g. Japanese characters in the path).
class PatchArchive {
public:
    static void                  LoadAll(const std::wstring& gameDir);
    static bool                  Has(const std::string& filename);
    static std::vector<uint8_t>  Extract(const std::string& filename);
    static void                  Unload();
    static bool                  IsBlacklisted(const std::string& filename);

private:
    static std::string Normalize(const std::string& name);

    struct Entry {
        size_t      archiveIndex;
        mz_uint     fileIndex;
        std::string sourceName;
    };

    static std::vector<mz_zip_archive>            s_archives;
    static std::vector<std::string>               s_names;
    static std::unordered_map<std::string, Entry> s_index;
};
