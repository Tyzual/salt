add_executable(
    tcp_client_test
    tcp_client_test.cpp
)

target_link_libraries(
    tcp_client_test
    salt
    gtest_main
)

target_compile_options(
    tcp_client_test PRIVATE
    -fno-access-control
)

target_include_directories(
    tcp_client_test PRIVATE
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)

add_executable(
    error_code_test
    error_code_test.cpp
)

target_link_libraries(
    error_code_test
    salt
    gtest_main
)

target_compile_options(
    error_code_test PRIVATE
    -fno-access-control
)

target_include_directories(
    error_code_test PRIVATE
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)

include(GoogleTest)
gtest_discover_tests(tcp_client_test)
gtest_discover_tests(error_code_test)