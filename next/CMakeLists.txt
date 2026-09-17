cmake_minimum_required(VERSION 3.25)

# JX NEXT - C++ zone core and shared libraries.
# Build with a preset (see CMakePresets.json); VCPKG_ROOT must point at a git checkout of vcpkg.
project(jxnext VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JX_BUILD_TESTS "Build unit tests" ON)
option(JX_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# One interface target carries the warning policy so every module gets the same flags.
add_library(jx_warnings INTERFACE)
if(MSVC)
    target_compile_options(jx_warnings INTERFACE /W4 /permissive- /utf-8 /Zc:__cplusplus /EHsc)
    target_compile_definitions(jx_warnings INTERFACE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
    if(JX_WARNINGS_AS_ERRORS)
        target_compile_options(jx_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(jx_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
    if(JX_WARNINGS_AS_ERRORS)
        target_compile_options(jx_warnings INTERFACE -Werror)
    endif()
endif()

if(JX_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(server/common)
add_subdirectory(server/core)
add_subdirectory(server/proto)
add_subdirectory(server/net)
add_subdirectory(server/zone)
