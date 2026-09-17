# jx::common - logging, configuration, clock and typed ids shared by every C++ module.
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(jx_common STATIC
    src/log.cpp
    src/config.cpp
    src/frame.cpp
    src/process.cpp
)
add_library(jx::common ALIAS jx_common)

target_include_directories(jx_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(jx_common
    PUBLIC  fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json
    PRIVATE jx_warnings
)
if(WIN32)
    target_link_libraries(jx_common PRIVATE psapi)   # GetProcessMemoryInfo for jx::process_usage
endif()
target_compile_features(jx_common PUBLIC cxx_std_20)

if(JX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
