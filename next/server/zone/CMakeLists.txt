# jx_zone_core - simulation + gateway protocol (library, unit tested)
# jx_zone      - the executable
find_package(Lua REQUIRED)   # standard Lua 5.4 (vcpkg): the old Lua 4 scripts run through KLuaScript

add_library(jx_zone_core STATIC
    src/KRegion.cpp
    src/KMapData.cpp
    src/KLuaScript.cpp
    src/KScriptCache.cpp
    src/ScriptFuns.cpp
    src/KNpcTemplate.cpp
    src/KSubWorld.cpp
    src/KNpcAI.cpp
    src/KGameServer.cpp
)
add_library(jx::zone_core ALIAS jx_zone_core)
target_include_directories(jx_zone_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(jx_zone_core PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(jx_zone_core
    PUBLIC  jx::common jx::core jx::entity jx::proto jx::net
    PRIVATE jx_warnings ${LUA_LIBRARIES}
)

add_executable(jx_zone src/main.cpp)
target_link_libraries(jx_zone PRIVATE jx::zone_core jx_warnings)
set_target_properties(jx_zone PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)

if(JX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
