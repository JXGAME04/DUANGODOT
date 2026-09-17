# jx::entity - entity handles with a generation and the table that owns entities (MASTER SPEC 5, 36).
add_library(jx_entity STATIC
    src/EntityHandle.cpp
)
add_library(jx::entity ALIAS jx_entity)

target_include_directories(jx_entity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(jx_entity
    PUBLIC  jx::common
    PRIVATE jx_warnings
)
target_compile_features(jx_entity PUBLIC cxx_std_20)

if(JX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
