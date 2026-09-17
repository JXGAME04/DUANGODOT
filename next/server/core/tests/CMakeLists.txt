find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_core_tests
    test_ServerClock.cpp
    test_FixedTick.cpp
    test_Result.cpp
    test_CommandQueue.cpp
    test_Metrics.cpp
    test_ThreadPool.cpp
    test_JobSystem.cpp
)
target_link_libraries(jx_core_tests PRIVATE jx::core Catch2::Catch2WithMain jx_warnings)
catch_discover_tests(jx_core_tests)
