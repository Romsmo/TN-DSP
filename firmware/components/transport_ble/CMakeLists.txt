# NimBLE GATT peripheral + CBOR codec (target side). Implements transport::ITransport.
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

idf_component_register(
  SRCS
    # TODO(M7): ble_transport.cpp ble_gatt.cpp ble_advertising.cpp
    #           ${REPO_ROOT}/core/codec/cbor_state.cpp  (shared with host)
    ble_transport_placeholder.cpp
  INCLUDE_DIRS
    "."
    "${REPO_ROOT}"
  REQUIRES
    bt
    nvs_flash
)
