cmake_minimum_required (VERSION 3.23)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (warehouse CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)


set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/learn")
set(PROTO_FILE "${PROTO_DIR}/learn.proto")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(MAKE_DIRECTORY "${GENERATED_DIR}")

set(PROTO_SRCS
    "${GENERATED_DIR}/learn.pb.cc"
    "${GENERATED_DIR}/learn.grpc.pb.cc"
)

set(PROTO_HDRS
    "${GENERATED_DIR}/learn.pb.h"
    "${GENERATED_DIR}/learn.grpc.pb.h"
)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND $<TARGET_FILE:protobuf::protoc>
    ARGS
        "--proto_path=${PROTO_DIR}"
        "--cpp_out=${GENERATED_DIR}"
        "--grpc_out=${GENERATED_DIR}"
        "--plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
    VERBATIM
)

add_library(proto_generated STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_include_directories(proto_generated
    PUBLIC "${GENERATED_DIR}"
)

target_link_libraries(proto_generated
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

# =======================    PROJECT    ==========================

add_executable(learn_server 
    src/learn/server.cpp
    src/learn/server_run.cpp
)

add_executable(learn_client
    src/learn/client.cpp
    src/learn/client_run.cpp
)

target_link_libraries(learn_server PRIVATE proto_generated)
target_link_libraries(learn_client PRIVATE proto_generated)
target_compile_features(learn_client PRIVATE cxx_std_23)
target_compile_features(learn_server PRIVATE cxx_std_23)

# add_executable(warehouse
#     src/main.cpp

#     src/app/app.cpp

#     src/utils/logger.cpp
# )

# target_include_directories(warehouse PUBLIC src)

# target_link_libraries(warehouse PUBLIC
#     spdlog::spdlog
# )