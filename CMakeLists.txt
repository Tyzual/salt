cmake_minimum_required(VERSION 3.14)

project(salt VERSION 0.3.1
  DESCRIPTION "a c++ network library based on asio"
  LANGUAGES CXX)

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
option(INSTALL_GTEST "Enable installation of googletest." OFF)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_compile_definitions("$<$<CONFIG:Debug>:ENABLE_LOG>")

add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(test)

include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "TyzuaL")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "a c++ network library based on asio")
set(CPACK_PACKAGE_VERSION_MAJOR "${salt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${salt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${salt_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.md")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  /.git
  /.vscode
  /.github
  /build
  /.gitmodules
)
include(CPack)
