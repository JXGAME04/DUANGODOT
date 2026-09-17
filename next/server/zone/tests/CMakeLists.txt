find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_zone_tests
    test_aoi.cpp
    test_world.cpp
    test_zone_server.cpp
)
target_link_libraries(jx_zone_tests PRIVATE jx::zone_core Catch2::Catch2WithMain jx_warnings)
catch_discover_tests(jx_zone_tests)
