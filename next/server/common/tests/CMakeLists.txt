find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_common_tests
    test_log.cpp
    test_config.cpp
    test_clock.cpp
    test_ids.cpp
)
target_link_libraries(jx_common_tests PRIVATE jx::common Catch2::Catch2WithMain jx_warnings)
catch_discover_tests(jx_common_tests)
