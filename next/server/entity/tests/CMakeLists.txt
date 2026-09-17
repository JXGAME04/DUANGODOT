find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(jx_entity_tests
    test_EntityHandle.cpp
    test_EntityTable.cpp
)
target_link_libraries(jx_entity_tests PRIVATE jx::entity Catch2::Catch2WithMain jx_warnings)
catch_discover_tests(jx_entity_tests)
