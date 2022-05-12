find_package(Threads)

# add_definitions(-DDISABLE_LOG)

add_library(salt
            salt/core/asio_io_context_thread.cpp
            salt/core/asio_io_context_thread.h
            salt/core/connection_handle.h
            salt/core/error.cpp
            salt/core/error.h
            salt/core/log.h
            salt/packet_assemble/packet_assemble.h
            salt/core/shared_asio_io_context_thread.cpp
            salt/core/shared_asio_io_context_thread.h
            salt/core/tcp_connection_handle.cpp
            salt/core/tcp_connection_handle.h
            salt/core/tcp_connection.cpp
            salt/core/tcp_connection.h
            salt/core/tcp_server.cpp
            salt/core/tcp_server.h
            salt/core/tcp_client.cpp
            salt/core/tcp_client.h
            salt/util/call_back_wrapper.h
)

configure_file(salt/version.h.in salt/version.h)

target_include_directories(salt PRIVATE
                            "${CMAKE_CURRENT_BINARY_DIR}"
                            "${CMAKE_CURRENT_SOURCE_DIR}"
                            "${asio_header}"
)

target_include_directories(salt INTERFACE
                            "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
                            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                            "$<INSTALL_INTERFACE:include>"
)

target_link_libraries(salt PRIVATE ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS salt EXPORT salt DESTINATION lib)
install(FILES
            salt/core/asio_io_context_thread.h
            salt/core/connection_handle.h
            salt/core/error.h
            salt/core/log.h
            salt/packet_assemble/packet_assemble.h
            salt/core/shared_asio_io_context_thread.h
            salt/core/tcp_connection_handle.h
            salt/core/tcp_connection.h
            salt/core/tcp_server.h
            salt/core/tcp_client.h
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