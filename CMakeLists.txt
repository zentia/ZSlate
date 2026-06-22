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

# Create static library
add_library(ZSlate STATIC
    ${ZSLATE_HEADERS}
    ${ZSLATE_SOURCES}
)

target_include_directories(ZSlate PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
