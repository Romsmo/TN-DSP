# The application component.
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

set(APP_SRCS app_main.cpp)
set(APP_REQUIRES hal_esp nvs_flash esp_timer)

# core/ is platform-independent and always built in.
file(GLOB CORE_SRCS
  "${REPO_ROOT}/core/model/*.cpp"
  "${REPO_ROOT}/core/catalog/*.cpp"
  "${REPO_ROOT}/core/settings/*.cpp"
  "${REPO_ROOT}/core/state/*.cpp"
  "${REPO_ROOT}/core/audio/*.cpp"
  "${REPO_ROOT}/core/codec/*.cpp")
list(APPEND APP_SRCS ${CORE_SRCS})

if(CONFIG_BDSP_ENABLE_DISPLAY)
  file(GLOB_RECURSE UI_SRCS
    "${REPO_ROOT}/ui/theme/*.cpp" "${REPO_ROOT}/ui/theme/fonts/*.c"
    "${REPO_ROOT}/ui/widgets/*.cpp" "${REPO_ROOT}/ui/screens/*.cpp"
    "${REPO_ROOT}/ui/overlays/*.cpp" "${REPO_ROOT}/ui/i18n/*.cpp"
    "${REPO_ROOT}/ui/assets/signs/*.c" "${REPO_ROOT}/ui/assets/signs/*.cpp"
    "${REPO_ROOT}/ui/assets/icons/*.c" "${REPO_ROOT}/ui/assets/icons/*.cpp")
  file(GLOB TRANS_SRCS "${REPO_ROOT}/transport/mock/*.cpp")
  list(APPEND APP_SRCS ${UI_SRCS} ${TRANS_SRCS})
  list(APPEND APP_REQUIRES lvgl)
endif()

idf_component_register(
  SRCS ${APP_SRCS}
  INCLUDE_DIRS "." "${REPO_ROOT}"
  REQUIRES ${APP_REQUIRES}
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
