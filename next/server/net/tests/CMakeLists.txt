find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_net_tests
    test_net.cpp
)
target_link_libraries(jx_net_tests PRIVATE jx::net Catch2::Catch2WithMain jx_warnings)
catch_discover_tests(jx_net_tests)
