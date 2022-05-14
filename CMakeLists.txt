cmake_minimum_required(VERSION 3.14)
project(salt VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
option(INSTALL_GTEST "Enable installation of googletest." OFF)
FetchContent_MakeAvailable(googletest)

set(asio_header "${CMAKE_SOURCE_DIR}/asio/asio/include" CACHE STRING "asio header directory(directory that contain asio.hpp)")
message("asio header dir:${asio_header}")

enable_testing()

# add_definitions(-DDISABLE_LOG)

add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(test)

include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_NAME "${PROJECT_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${salt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${salt_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)
