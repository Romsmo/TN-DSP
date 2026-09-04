# NimBLE GATT peripheral (target side).
#
# REQUIRES is unconditional: a `CONFIG_*`-gated REQUIRES breaks IDF's early
# component-graph expansion (same reason bdsp/hal_esp list `lvgl` unconditionally).
# ble_transport.cpp is only compiled — and the BT stack only linked — when
# CONFIG_BDSP_ENABLE_BLE is set.
set(REPO "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(SRCS "")
if(CONFIG_BDSP_ENABLE_BLE)
  set(SRCS ble_transport.cpp)
endif()

idf_component_register(
  SRCS ${SRCS}
  INCLUDE_DIRS "." "${REPO}"
  REQUIRES nvs_flash bt esp_timer bdsp app_update mbedtls
)

if(CONFIG_BDSP_ENABLE_BLE)
  target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
endif()
