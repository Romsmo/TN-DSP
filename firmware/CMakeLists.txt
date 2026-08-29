# ESP-IDF project — Waveshare ESP32-C6-Touch-AMOLED-1.43
#
#   idf.py set-target esp32c6
#   idf.py build flash monitor
#
# NOTE: not buildable without ESP-IDF >= 5.3 in the environment. The host build
# (repo root CMakeLists.txt) does NOT include this directory.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
  "${CMAKE_CURRENT_LIST_DIR}/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Shared core/ + ui/ code is the host build's -Werror responsibility; don't let
# IDF's stricter component warnings (32-bit int format widths, etc.) fail it.
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format" APPEND)

project(blitzerdsp)
