# Concrete HAL for the ESP32-C6 board. Implements the interfaces in repo hal/.
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

idf_component_register(
  SRCS
    # TODO(M1): esp_display.cpp esp_touch.cpp esp_i2c_bus.cpp esp_io_expander.cpp
    #           esp_storage_sd.cpp esp_nvs.cpp esp_clock.cpp esp_buttons.cpp
    #           esp_rtc_pcf85063.cpp esp_imu_qmi8658.cpp esp_power.cpp
    hal_esp_placeholder.cpp
  INCLUDE_DIRS
    "include"
    "${REPO_ROOT}"
    "${REPO_ROOT}/firmware/main"
  REQUIRES
    esp_lcd
    esp_lcd_sh8601
    esp_io_expander_tca9554
    esp_driver_i2c
    esp_driver_spi
    esp_driver_gpio
    fatfs
    sdmmc
    nvs_flash
    esp_adc
)
