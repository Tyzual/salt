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
)

configure_file(salt/version.h.in salt/version.h)
target_include_directories(salt PUBLIC 
                            "${CMAKE_CURRENT_BINARY_DIR}" 
                            "${CMAKE_CURRENT_SOURCE_DIR}"
                            "${CMAKE_SOURCE_DIR}/asio"
)

target_link_libraries(salt PRIVATE ${CMAKE_THREAD_LIBS_INIT})