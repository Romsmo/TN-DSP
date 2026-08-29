# Concrete HAL for the ESP32-C6 board. Implements the interfaces in repo hal/.
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(SRCS
  esp_clock.cpp
  esp_nvs.cpp
  esp_i2c.cpp
  esp_io_expander.cpp
  esp_buttons.cpp
  esp_misc.cpp
)
set(REQUIRES driver esp_driver_i2c esp_driver_gpio esp_driver_spi esp_timer nvs_flash)

if(CONFIG_BDSP_ENABLE_DISPLAY)
  list(APPEND SRCS esp_display.cpp esp_touch.cpp esp_lvgl.cpp)
  list(APPEND REQUIRES esp_lcd esp_lcd_sh8601 lvgl)
endif()

idf_component_register(
  SRCS ${SRCS}
  INCLUDE_DIRS "include" "." "${REPO_ROOT}" "${REPO_ROOT}/firmware/main"
  REQUIRES ${REQUIRES}
)
