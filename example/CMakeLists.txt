add_executable(
    tcp_echo_server
    tcp_echo_server.cpp
)

target_link_libraries(
    tcp_echo_server
    salt
)

target_include_directories(
    tcp_echo_server PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)

add_executable(
    tcp_echo_client
    tcp_echo_client.cpp
)

target_link_libraries(
    tcp_echo_client
    salt
)

target_include_directories(
    tcp_echo_client PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)