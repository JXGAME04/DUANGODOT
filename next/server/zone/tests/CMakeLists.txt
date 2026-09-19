find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_zone_tests
    test_KRegion.cpp
    test_KMapData.cpp
    test_KSubWorld.cpp
    test_KInterest.cpp
    test_KGameServer.cpp
    test_KWorldScheduler.cpp
    test_KMath.cpp
    test_KNpcAI.cpp
    test_KItem.cpp
    test_KLuaScript.cpp
    test_KTrap.cpp
    test_KPlayer.cpp
    test_KSkill.cpp
    test_KNpcDamage.cpp
    test_KMissle.cpp
    test_KAutoSkill.cpp
    test_KSkillList.cpp
    test_KNpcCommand.cpp
    test_KNpcGold.cpp
    test_KPlayerTeam.cpp
    test_KPlayerChat.cpp
)
target_link_libraries(jx_zone_tests PRIVATE jx::zone_core Catch2::Catch2WithMain jx_warnings)
# so a test can look for the converted script tree in data/, wherever the build directory is
target_compile_definitions(jx_zone_tests PRIVATE JX_NEXT_DIR="${CMAKE_SOURCE_DIR}")
catch_discover_tests(jx_zone_tests)
