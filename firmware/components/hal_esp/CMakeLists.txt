# Concrete HAL for the ESP32-C6 board. Implements the interfaces in repo hal/.
set(REPO "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(SRCS
  esp_clock.cpp
  esp_nvs.cpp
  esp_i2c.cpp
  esp_io_expander.cpp
  esp_buttons.cpp
  esp_misc.cpp
  esp_display.cpp
  esp_touch.cpp
  esp_lvgl.cpp
  esp_storage.cpp
  esp_audio.cpp
)

idf_component_register(
  SRCS ${SRCS}
  INCLUDE_DIRS "include" "." "${REPO}" "${REPO}/firmware/main"
  REQUIRES driver esp_driver_i2c esp_driver_gpio esp_driver_spi esp_driver_sdspi esp_driver_i2s
           esp_timer nvs_flash esp_lcd esp_lcd_sh8601 lvgl fatfs sdmmc esp_codec_dev
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
