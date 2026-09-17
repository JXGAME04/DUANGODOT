find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_zone_tests
    test_KRegion.cpp
    test_KMapData.cpp
    test_KSubWorld.cpp
    test_KGameServer.cpp
    test_KWorldScheduler.cpp
    test_KMath.cpp
    test_KNpcAI.cpp
    test_KLuaScript.cpp
    test_KTrap.cpp
)
target_link_libraries(jx_zone_tests PRIVATE jx::zone_core Catch2::Catch2WithMain jx_warnings)
catch_discover_tests(jx_zone_tests)
