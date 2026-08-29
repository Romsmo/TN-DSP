# NimBLE GATT peripheral + CBOR codec (target side). Implements transport::ITransport.
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

idf_component_register(
  SRCS
    ble_transport.cpp
    "${REPO_ROOT}/core/codec/protocol.cpp"   # shared with the host build
  INCLUDE_DIRS
    "."
    "${REPO_ROOT}"
  REQUIRES
    bt
    nvs_flash
)
