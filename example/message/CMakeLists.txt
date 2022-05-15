add_executable(
    tcp_message_client
    tcp_message_client.cpp
)

target_link_libraries(
    tcp_message_client
    salt
)

target_include_directories(
    tcp_message_client PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)