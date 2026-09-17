# jx::proto - C++ classes generated from ../../proto/jx/*.proto (Protocol V2).
# Generated code is third-party output: it is compiled without the project warning policy and
# exposed as a SYSTEM include so consumers keep /WX.
find_package(protobuf CONFIG REQUIRED)

set(JX_PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)
file(GLOB JX_PROTO_FILES CONFIGURE_DEPENDS ${JX_PROTO_DIR}/jx/*.proto)

add_library(jx_proto STATIC ${JX_PROTO_FILES})
add_library(jx::proto ALIAS jx_proto)
target_link_libraries(jx_proto PUBLIC protobuf::libprotobuf)
target_compile_features(jx_proto PUBLIC cxx_std_20)

set(JX_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
protobuf_generate(
    TARGET jx_proto
    LANGUAGE cpp
    IMPORT_DIRS ${JX_PROTO_DIR}
    PROTOC_OUT_DIR ${JX_PROTO_GEN_DIR}
)
target_include_directories(jx_proto SYSTEM PUBLIC ${JX_PROTO_GEN_DIR})
if(MSVC)
    target_compile_options(jx_proto PRIVATE /W0 /utf-8)
else()
    target_compile_options(jx_proto PRIVATE -w)
endif()
