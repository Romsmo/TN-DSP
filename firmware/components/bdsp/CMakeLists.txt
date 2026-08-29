# bdsp — the platform-independent core/ + ui/ + mock transport, as one ESP-IDF
# component. Sources live in the repo root, shared verbatim with the host build.
#
# Everything is compiled unconditionally (dead code is dropped at link time);
# CONFIG_BDSP_ENABLE_DISPLAY only decides whether app_main instantiates the UI.
set(REPO "${CMAKE_CURRENT_LIST_DIR}/../../..")

file(GLOB_RECURSE CORE_SRCS  "${REPO}/core/*.cpp")
file(GLOB         TRANS_SRCS "${REPO}/transport/mock/*.cpp")
file(GLOB_RECURSE UI_SRCS
  "${REPO}/ui/theme/*.cpp"   "${REPO}/ui/theme/fonts/*.c"
  "${REPO}/ui/widgets/*.cpp" "${REPO}/ui/screens/*.cpp"
  "${REPO}/ui/overlays/*.cpp" "${REPO}/ui/i18n/*.cpp"
  "${REPO}/ui/assets/signs/*.c"  "${REPO}/ui/assets/signs/*.cpp"
  "${REPO}/ui/assets/icons/*.c"  "${REPO}/ui/assets/icons/*.cpp")

idf_component_register(
  SRCS ${CORE_SRCS} ${TRANS_SRCS} ${UI_SRCS}
  INCLUDE_DIRS "${REPO}"
  REQUIRES esp_timer lvgl
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)
# This is our shared code; the host build is the strict -Werror gate. Don't let
# IDF's stricter component warnings (e.g. -Werror=format on 32-bit int widths) fail it.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)

file(GLOB GEN_C "${REPO}/ui/theme/fonts/*.c" "${REPO}/ui/assets/signs/*.c"
                "${REPO}/ui/assets/icons/*.c" "${REPO}/core/audio/fallback_tones.cpp")
set_source_files_properties(${GEN_C} PROPERTIES COMPILE_FLAGS "-w")
