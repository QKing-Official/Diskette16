cmake_minimum_required(VERSION 3.20)

project(Diskette16 VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

enable_testing()

add_library(diskette16_core
  src/cart.cpp
  src/cart_format.cpp
  src/script_vm.cpp
)

target_include_directories(diskette16_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(diskette16_editor
  src/editor_main.cpp
)

target_link_libraries(diskette16_editor
  PRIVATE
    diskette16_core
    raylib
)

if(UNIX AND NOT APPLE)
  target_link_libraries(diskette16_editor PRIVATE m pthread dl)
endif()

add_executable(diskette16_packer
  src/packer_main.cpp
)

target_link_libraries(diskette16_packer
  PRIVATE
    diskette16_core
)

add_executable(diskette16_tests
  tests/all_tests.cpp
  tests/script_vm_test.cpp
  tests/cart_format_test.cpp
)

target_link_libraries(diskette16_tests
  PRIVATE
    diskette16_core
)

add_test(NAME script_vm COMMAND diskette16_tests)
