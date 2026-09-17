# jx_zone_core - simulation + gateway protocol (library, unit tested)
# jx_zone      - the executable
add_library(jx_zone_core STATIC
    src/aoi.cpp
    src/world.cpp
    src/zone_server.cpp
)
add_library(jx::zone_core ALIAS jx_zone_core)
target_include_directories(jx_zone_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(jx_zone_core
    PUBLIC  jx::common jx::proto jx::net
    PRIVATE jx_warnings
)

add_executable(jx_zone src/main.cpp)
target_link_libraries(jx_zone PRIVATE jx::zone_core jx_warnings)
set_target_properties(jx_zone PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)

if(JX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
