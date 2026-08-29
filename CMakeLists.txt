# Host build: unit tests + LVGL-SDL simulator.
# The firmware is built separately with ESP-IDF (see firmware/).
cmake_minimum_required(VERSION 3.20)
project(blitzerdsp_host LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(BDSP_BUILD_TESTS "Build host unit tests" ON)
option(BDSP_BUILD_SIM   "Build the LVGL-SDL simulator" ON)
option(BDSP_WERROR      "Treat warnings as errors" OFF)

add_library(bdsp_warnings INTERFACE)
if(MSVC)
  target_compile_options(bdsp_warnings INTERFACE /W4 $<$<BOOL:${BDSP_WERROR}>:/WX>)
else()
  target_compile_options(bdsp_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    $<$<BOOL:${BDSP_WERROR}>:-Werror>)
endif()

add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# bdsp_core: platform-independent logic. Defined in core/, links no platform libs.
add_subdirectory(core)

if(BDSP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(BDSP_BUILD_SIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sim/CMakeLists.txt)
  find_package(SDL2 QUIET)
  if(SDL2_FOUND OR TARGET SDL2::SDL2)
    add_subdirectory(sim)
  else()
    message(STATUS "SDL2 not found - skipping simulator target (set BDSP_BUILD_SIM=OFF to silence)")
  endif()
endif()
