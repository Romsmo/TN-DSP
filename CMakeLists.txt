# Host build: unit tests + LVGL-SDL simulator.
# The firmware is built separately with ESP-IDF (see firmware/).
cmake_minimum_required(VERSION 3.20)
project(blitzerdsp_host LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BDSP_BUILD_TESTS "Build host unit tests" ON)
option(BDSP_BUILD_SIM   "Build the LVGL-SDL simulator" ON)
option(BDSP_BUILD_UI    "Build the LVGL ui/ library (needs LVGL fetch)" ON)
option(BDSP_WERROR      "Treat warnings as errors" OFF)

# Strict warnings for our own platform-independent code (core/, tests).
add_library(bdsp_warnings INTERFACE)
# Relaxed set for code that talks to LVGL's C API heavily (ui/, sim/).
add_library(bdsp_warnings_lite INTERFACE)
if(MSVC)
  target_compile_options(bdsp_warnings INTERFACE /W4 $<$<BOOL:${BDSP_WERROR}>:/WX>)
  target_compile_options(bdsp_warnings_lite INTERFACE /W3 $<$<BOOL:${BDSP_WERROR}>:/WX>)
else()
  target_compile_options(bdsp_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    $<$<BOOL:${BDSP_WERROR}>:-Werror>)
  target_compile_options(bdsp_warnings_lite INTERFACE
    -Wall -Wextra -Wshadow
    $<$<BOOL:${BDSP_WERROR}>:-Werror>)
endif()

add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# bdsp_core: platform-independent logic. Defined in core/, links no platform libs.
add_subdirectory(core)

set(BDSP_HAVE_SDL OFF)
if(BDSP_BUILD_SIM)
  find_package(SDL2 QUIET)
  if(SDL2_FOUND OR TARGET SDL2::SDL2)
    set(BDSP_HAVE_SDL ON)
  else()
    message(STATUS "SDL2 not found - simulator disabled")
  endif()
endif()

if(BDSP_BUILD_UI OR BDSP_HAVE_SDL)
  include(lvgl)                       # FetchContent LVGL 9.2.2
  add_subdirectory(ui)
endif()

if(BDSP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(BDSP_HAVE_SDL AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sim/CMakeLists.txt)
  add_subdirectory(sim)
endif()
