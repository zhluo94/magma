add_compile_options(-std=c++14)

set (CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-literal-suffix")
add_definitions(-DDEBUG_IS_ON=1)

# compile the needed protos
set(SUB_LTE_CPP_PROTOS subscriberdb)
set(SUB_LTE_GRPC_PROTOS subscriberdb)
set(SUB_ORC8R_CPP_PROTOS common)

list(APPEND PROTO_SRCS "")
list(APPEND PROTO_HDRS "")

create_proto_dir("orc8r" ORC8R_OUT_DIR)
create_proto_dir("lte" LTE_OUT_DIR)

generate_cpp_protos("${SUB_LTE_CPP_PROTOS}" "${PROTO_SRCS}"
  "${PROTO_HDRS}" ${LTE_PROTO_DIR} ${LTE_OUT_DIR})
generate_grpc_protos("${SUB_LTE_GRPC_PROTOS}" "${PROTO_SRCS}"
  "${PROTO_HDRS}" ${LTE_PROTO_DIR} ${LTE_OUT_DIR})

generate_cpp_protos("${SUB_ORC8R_CPP_PROTOS}" "${PROTO_SRCS}"
  "${PROTO_HDRS}" ${ORC8R_PROTO_DIR} ${ORC8R_OUT_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(LIB_SUB_PROXY
  SubscriberClient.cpp
  subscriber_client_api.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
  )

target_link_libraries(LIB_SUB_PROXY
  COMMON
  LIB_RPC_CLIENT ${ASYNC_GRPC} ${SERVICE_REGISTRY}
  ${CONFIG} LIB_BSTR LIB_HASHTABLE
)
target_include_directories(LIB_SUB_PROXY PUBLIC
  ${MAGMA_LIB_DIR}/async_grpc
  ${MAGMA_LIB_DIR}/config
  ${MAGMA_LIB_DIR}/service_registry
  ${CMAKE_CURRENT_SOURCE_DIR}
)
