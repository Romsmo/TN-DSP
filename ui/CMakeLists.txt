# bdsp_ui — LVGL screens, widgets, theme, i18n. No ESP-IDF / SDL includes; the
# display and input come from hal/ (target) or the SDL backend wired in sim/.

file(GLOB   UI_FONT_SRCS  CONFIGURE_DEPENDS theme/fonts/*.c)
file(GLOB_RECURSE UI_SRCS CONFIGURE_DEPENDS
  theme/*.cpp
  widgets/*.cpp
  screens/*.cpp
  overlays/*.cpp
  i18n/*.cpp
  gesture/*.cpp
)

add_library(bdsp_ui STATIC ${UI_SRCS} ${UI_FONT_SRCS})
target_include_directories(bdsp_ui PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(bdsp_ui PUBLIC cxx_std_17)
target_link_libraries(bdsp_ui PUBLIC bdsp_core lvgl)

# ui/ talks to LVGL's C API a lot -> relaxed warning set (no -Wconversion).
if(TARGET bdsp_warnings_lite)
  target_link_libraries(bdsp_ui PRIVATE bdsp_warnings_lite)
endif()
# The generated font .c files are third-party output.
set_source_files_properties(${UI_FONT_SRCS} PROPERTIES COMPILE_FLAGS "-w")
