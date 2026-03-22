cmake_minimum_required(VERSION 3.15)
project(agepatch CXX C)

# Must be built as 32-bit: cmake -A Win32 ..

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(version SHARED
    src/dllmain.cpp
    src/version_proxy.cpp
    src/hooks.cpp
    src/patch_archive.cpp
    src/vfile.cpp
    src/patch_log.cpp
    deps/miniz/miniz.c
)

set_source_files_properties(deps/miniz/miniz.c PROPERTIES LANGUAGE C)

target_include_directories(version PRIVATE
    src
    deps/miniz
    deps/minhook/include
)

target_link_libraries(version PRIVATE
    "${CMAKE_SOURCE_DIR}/deps/minhook/lib/libMinHook.x86.lib"
)

set_target_properties(version PROPERTIES
    OUTPUT_NAME "version"
    PREFIX ""
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)

target_link_options(version PRIVATE /INCREMENTAL:NO)
