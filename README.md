# Eushully Fanpatch Loader

A `version.dll` proxy for Eushully AGE engine games that loads patched
assets from `*.fanpatch` files (standard ZIPs with a renamed extension)
without any loose files cluttering the game directory.

[![Discord](https://img.shields.io/discord/1285796791375106108?style=for-the-badge&logo=discord&label=Discord)](https://discord.gg/SxcVKVzv4x) 

## Distribution

Drop into the game directory:
```
version.dll      ← this project's output
patch.fanpatch   ← your patched assets (a ZIP renamed to .fanpatch)
```

Multiple `.fanpatch` files are supported. They load alphabetically —
last one wins on filename conflicts. Use numeric prefixes to control
priority: `01_english.fanpatch`, `02_uncensor.fanpatch`.

To uninstall: delete both files. The base game is completely untouched.

## Creating a .fanpatch file

A `.fanpatch` is a ZIP containing patched assets flat in the root
(no subdirectories). Filenames should be uppercase to match how the
engine requests them e.g. `CHECKCONFIG.BIN` not `checkconfig.bin`.
The hook normalises to uppercase at runtime so case mismatches are
handled automatically.

Using 7-Zip: select files → Add to archive → ZIP → rename to `.fanpatch`.

## Building

Prerequisites:
- Visual Studio 2019/2022 with C++ workload
- CMake 3.15+
- MinHook: https://github.com/TsudaKageyu/minhook
- miniz: https://github.com/richgel999/miniz

Dependency layout:
```
deps/
  minhook/
    include/MinHook.h
    lib/libMinHook.x86.lib
  miniz/
    miniz.h
    miniz.c
```

Build (must be 32-bit):
```bat
mkdir build && cd build
cmake -A Win32 ..
cmake --build . --config Release
```

Output: `build/Release/version.dll`

---

## Disclaimer

This project was developed with the assistance of [Claude](https://claude.ai), an AI assistant made by Anthropic.
