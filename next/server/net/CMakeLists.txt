# jx::net - framed TCP on standalone Asio (header-only from vcpkg).
find_package(asio CONFIG REQUIRED)

add_library(jx_net STATIC
    src/KSocket.cpp
)
add_library(jx::net ALIAS jx_net)
target_include_directories(jx_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(jx_net
    PUBLIC  jx::common jx::proto asio::asio
    PRIVATE jx_warnings
)
target_compile_definitions(jx_net PUBLIC ASIO_STANDALONE ASIO_NO_DEPRECATED)
if(WIN32)
    target_compile_definitions(jx_net PUBLIC _WIN32_WINNT=0x0A00)
    target_link_libraries(jx_net PUBLIC ws2_32 mswsock)
endif()

if(JX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
