# jx::core - the server runtime foundation of the MASTER SPEC: server clock and tick, result /
# error, command and event queues, thread pool, job system, metrics.  No gameplay lives here.
add_library(jx_core STATIC
    src/Result.cpp
    src/Metrics.cpp
    src/FixedTick.cpp
    src/ThreadPool.cpp
    src/JobSystem.cpp
)
add_library(jx::core ALIAS jx_core)

target_include_directories(jx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(jx_core
    PUBLIC  jx::common
    PRIVATE jx_warnings
)
target_compile_features(jx_core PUBLIC cxx_std_20)
find_package(Threads REQUIRED)
target_link_libraries(jx_core PUBLIC Threads::Threads)

if(JX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
