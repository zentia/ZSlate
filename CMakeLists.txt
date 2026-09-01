cmake_minimum_required(VERSION 3.20)
project(ZSlate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(ZSLATE_BUILD_TESTS "Build tests" OFF)
option(ZSLATE_BUILD_EXAMPLES "Build examples" OFF)

# Source files
file(GLOB_RECURSE ZSLATE_HEADERS include/*.h)
file(GLOB_RECURSE ZSLATE_SOURCES src/*.cpp)

# The D3D11 renderer backend is Windows-only (uses HWND / ID3D11* / DXGI types).
# Exclude it on non-Windows platforms so clang (Android/iOS) doesn't try to compile it.
if(NOT WIN32)
    list(FILTER ZSLATE_HEADERS EXCLUDE REGEX "Platform[/\\]D3D11[/\\]")
    list(FILTER ZSLATE_SOURCES EXCLUDE REGEX "Platform[/\\]D3D11[/\\]")
endif()

# Create static library
add_library(ZSlate STATIC
    ${ZSLATE_HEADERS}
    ${ZSLATE_SOURCES}
)

# When ZSlate is built as a subdirectory of ZEngine, the parent project sets
# CMAKE_ARCHIVE_OUTPUT_DIRECTORY_<CONFIG> to a single unified directory for
# every config (see ZEngine/CMakeLists.txt "ZEngine unified output directory").
# Without a per-config override, Debug and Release ZSlate.lib overwrite each
# other, and a stale .lib from the other config is linked into consumers ->
# LNK2038 "_ITERATOR_DEBUG_LEVEL"/"RuntimeLibrary" mismatch. Mirror what every
# other ZEngine static library does via z_set_config_archive_dirs. Guarded so
# standalone ZSlate builds (where the helper is undefined) are unaffected.
if(COMMAND z_set_config_archive_dirs)
    z_set_config_archive_dirs(ZSlate "${CMAKE_CURRENT_BINARY_DIR}")
endif()

target_include_directories(ZSlate PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

# Compile options
target_compile_definitions(ZSlate PUBLIC
    ZSLATE_EXPORTS
)

# Platform-specific settings
if(MSVC)
    target_compile_options(ZSlate PRIVATE
        /W4
        /permissive-
        /experimental:preprocessor
        /Zc:__cplusplus
        /utf-8
    )
    target_compile_definitions(ZSlate PRIVATE
        _USE_MATH_DEFINES
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ZSlate PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Install rules
install(TARGETS ZSlate
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ZSlate
    DESTINATION include
)

# Tests
if(ZSLATE_BUILD_TESTS)
    enable_testing()
    # Add tests here
endif()

# Examples
if(ZSLATE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
