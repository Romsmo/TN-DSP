# bdsp_sim — desktop build of ui/ + core/ against LVGL's SDL backend.
# 466x466 window, mouse = touch, number keys switch scenarios, 'S' dumps a PNG.

add_executable(bdsp_sim
  main.cpp
  scenarios.cpp
  screenshot.cpp
)
target_compile_definitions(bdsp_sim PRIVATE BDSP_SIM=1)
target_include_directories(bdsp_sim PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(bdsp_sim PRIVATE bdsp_ui bdsp_core lvgl bdsp_warnings_lite)

# main.cpp uses SDL_MAIN_HANDLED, so we deliberately do NOT link SDL2main.
if(TARGET SDL2::SDL2)
  target_link_libraries(bdsp_sim PRIVATE SDL2::SDL2)
else()
  target_include_directories(bdsp_sim PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(bdsp_sim PRIVATE ${SDL2_LIBRARIES})
endif()

set(BDSP_SCREENSHOT_DIR "${CMAKE_SOURCE_DIR}/docs/screenshots" CACHE PATH "")
target_compile_definitions(bdsp_sim PRIVATE
  BDSP_SCREENSHOT_DIR="${BDSP_SCREENSHOT_DIR}")
