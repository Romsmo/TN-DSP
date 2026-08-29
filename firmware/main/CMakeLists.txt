# The application component. Pulls in the platform-independent core/ + ui/ + transport/
# sources from the repo root (they contain no IDF includes) and wires them to the ESP HAL.

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

file(GLOB_RECURSE CORE_SRCS  "${REPO_ROOT}/core/*.cpp")
file(GLOB_RECURSE UI_SRCS    "${REPO_ROOT}/ui/*.cpp")
file(GLOB_RECURSE TRANS_SRCS "${REPO_ROOT}/transport/mock/*.cpp"
                             "${REPO_ROOT}/transport/serial/*.cpp")

idf_component_register(
  SRCS
    app_main.cpp
    ${CORE_SRCS}
    ${UI_SRCS}
    ${TRANS_SRCS}
  INCLUDE_DIRS
    "."
    "${REPO_ROOT}"
  REQUIRES
    hal_esp
    transport_ble
    lvgl
    esp_lvgl_port
    nvs_flash
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
