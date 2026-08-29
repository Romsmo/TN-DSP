# Fetch LVGL for the host build (firmware gets it via the ESP-IDF component manager).
# Pinned to match firmware/main/idf_component.yml (~9.2.0).
include(FetchContent)

set(LV_CONF_BUILD_DISABLE_EXAMPLES ON  CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS    ON  CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG        v9.2.2
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(lvgl)

# LVGL globs src/*.S unconditionally; the NEON/Helium ARM assembly does not build
# on x86 (or the RISC-V C6). Drop assembly sources - LV_USE_DRAW_SW_ASM is NONE.
get_target_property(_lvgl_srcs lvgl SOURCES)
list(FILTER _lvgl_srcs EXCLUDE REGEX "\\.[sS]$")
set_target_properties(lvgl PROPERTIES SOURCES "${_lvgl_srcs}")

# Our lv_conf.h lives in ui/. Tell LVGL to find it by simple include + add the path.
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE BDSP_HOST=1)
target_include_directories(lvgl SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/ui)

# Wire SDL into LVGL so its src/drivers/sdl/*.c (guarded by LV_USE_SDL) links.
find_package(SDL2 QUIET)
if(TARGET SDL2::SDL2)
  target_link_libraries(lvgl PUBLIC SDL2::SDL2)
elseif(SDL2_FOUND)
  target_include_directories(lvgl SYSTEM PUBLIC ${SDL2_INCLUDE_DIRS})
  target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES})
endif()
