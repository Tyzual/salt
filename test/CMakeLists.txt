add_executable(
    header_body_assemble_test
    header_body_assemble_test.cpp
)

target_link_libraries(
    header_body_assemble_test
    salt
    gtest_main
)

target_compile_options(
    header_body_assemble_test PRIVATE
    -fno-access-control
)

target_include_directories(
    header_body_assemble_test PRIVATE
    "${asio_header}"
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
    "${asio_header}"
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)

include(GoogleTest)
gtest_discover_tests(error_code_test)
gtest_discover_tests(header_body_assemble_test)