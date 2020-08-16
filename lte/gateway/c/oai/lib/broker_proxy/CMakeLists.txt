add_compile_options(-std=c++14)

set (CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-literal-suffix")
add_definitions(-DDEBUG_IS_ON=1)

# compile the needed protos
set(BROKER_CPP_PROTOS common brokerd)
set(BROKER_GRPC_PROTOS common brokerd)
set(BROKER_LTE_CPP_PROTOS mconfig/mconfigs)

list(APPEND PROTO_SRCS "")
list(APPEND PROTO_HDRS "")

create_proto_dir("broker" BROKER_OUT_DIR)
create_proto_dir("lte" LTE_OUT_DIR)

generate_cpp_protos("${BROKER_CPP_PROTOS}" "${PROTO_SRCS}"
  "${PROTO_HDRS}" ${BROKER_PROTO_DIR} ${BROKER_OUT_DIR})
generate_grpc_protos("${BROKER_GRPC_PROTOS}" "${PROTO_SRCS}"
  "${PROTO_HDRS}" ${BROKER_PROTO_DIR} ${BROKER_OUT_DIR})

generate_cpp_protos("${BROKER_LTE_CPP_PROTOS}" "${PROTO_SRCS}"
  "${PROTO_HDRS}" ${LTE_PROTO_DIR} ${LTE_OUT_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(LIB_BROKER_PROXY
  brokerClient.cpp
  broker_client_api.cpp
  itti_msg_to_proto_msg.cpp
  proto_msg_to_itti_msg.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
  )

target_link_libraries(LIB_BROKER_PROXY
  COMMON
  LIB_RPC_CLIENT ${ASYNC_GRPC} ${SERVICE_REGISTRY}
  ${CONFIG} LIB_BSTR LIB_HASHTABLE
)
target_include_directories(LIB_BROKER_PROXY PUBLIC
  ${MAGMA_LIB_DIR}/async_grpc
  ${MAGMA_LIB_DIR}/config
  ${MAGMA_LIB_DIR}/service_registry
  ${CMAKE_CURRENT_SOURCE_DIR}
)
