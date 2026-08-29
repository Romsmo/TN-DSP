# NimBLE GATT peripheral (target side). Only real when CONFIG_BDSP_ENABLE_BLE=y;
# otherwise this is an empty component so the BT stack isn't pulled in.
set(REPO "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(SRCS "")
set(REQ nvs_flash)
if(CONFIG_BDSP_ENABLE_BLE)
  set(SRCS ble_transport.cpp)
  list(APPEND REQ bt bdsp)
endif()

idf_component_register(
  SRCS ${SRCS}
  INCLUDE_DIRS "." "${REPO}"
  REQUIRES ${REQ}
)
