find_package(Threads)

add_library(salt
            salt/asio_io_context_thread.cpp
            salt/asio_io_context_thread.h
            salt/connection_handle.h
            salt/error.cpp
            salt/error.h
            salt/log.h
            salt/packet_assemble.h
            salt/shared_asio_io_context_thread.cpp
            salt/shared_asio_io_context_thread.h
            salt/tcp_connection_handle.cpp
            salt/tcp_connection_handle.h
            salt/tcp_connection.cpp
            salt/tcp_connection.h
            salt/tcp_server.cpp
            salt/tcp_server.h
            salt/tcp_client.cpp
            salt/tcp_client.h
            salt/util/call_back_wrapper.h
)

configure_file(salt/version.h.in salt/version.h)

target_include_directories(salt PRIVATE 
                            "${CMAKE_CURRENT_BINARY_DIR}" 
                            "${CMAKE_CURRENT_SOURCE_DIR}"
                            "${CMAKE_SOURCE_DIR}/asio"
)

target_include_directories(salt INTERFACE 
                            "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>" 
                            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                            "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}:asio>"
)

target_link_libraries(salt PRIVATE ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS salt EXPORT salt DESTINATION lib)
install(FILES
            salt/asio_io_context_thread.h
            salt/connection_handle.h
            salt/error.h
            salt/log.h
            salt/packet_assemble.h
            salt/shared_asio_io_context_thread.h
            salt/tcp_connection_handle.h
            salt/tcp_connection.cpp
            salt/tcp_connection.h
            salt/tcp_server.cpp
            salt/tcp_server.h
            salt/tcp_client.cpp
            salt/tcp_client.h
            DESTINATION include/salt
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/salt/version.h" DESTINATION include/salt)
install(EXPORT salt 
    FILE salt.cmake 
    DESTINATION lib/cmake/salt
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/saltConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/salt"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/saltConfigVersion.cmake"
    VERSION "${salt_VERSION_MAJOR}.${salt_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/saltConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/saltConfigVersion.cmake"
    DESTINATION "lib/cmake/salt"
)

export(EXPORT salt
    FILE "${CMAKE_CURRENT_BINARY_DIR}/salt.cmake"
)