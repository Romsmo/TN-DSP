// ble_uuids.h — the ONE place BLE UUIDs are defined.
//
// PROVISIONAL. Base generated randomly for development; finalized at milestone M8
// (may change if Eifrig Media supplies a vendor base during licensing talks).
//
// Base:  079aXXXX-d673-469c-98a0-12c0c5eb43b3   (XXXX = 16-bit slot)
#pragma once

#define BDSP_BLE_UUID128(slot) \
    { 0xb3, 0x43, 0xeb, 0xc5, 0xc0, 0x12, 0xa0, 0x98, \
      0x9c, 0x46, 0x73, 0xd6, ((slot) & 0xff), (((slot) >> 8) & 0xff), 0x9a, 0x07 }
// (little-endian byte order for NimBLE ble_uuid128_t.value)

enum {
    BDSP_BLE_SLOT_SERVICE      = 0x0000,
    BDSP_BLE_SLOT_STATE        = 0x0001,  // App -> device, Write w/o Response, CBOR DisplayState
    BDSP_BLE_SLOT_EVENT        = 0x0002,  // device -> App, Notify, CBOR DeviceEvent
    BDSP_BLE_SLOT_DEVICE_INFO  = 0x0003,  // Read: proto ver, fw ver, caps bitmask, resolution
    BDSP_BLE_SLOT_SETTINGS     = 0x0004,  // Read/Write/Notify, CBOR Settings (bidirectional sync)
    BDSP_BLE_SLOT_TIME         = 0x0005,  // Write: {epochS, tzOffsetMin} -> PCF85063
    BDSP_BLE_SLOT_UI_CONFIG    = 0x0006,  // Read/Write/Notify, CBOR UiConfig (dev feature switches)
    BDSP_BLE_SLOT_OTA          = 0x0007,  // Write/Notify, chunked firmware image (M9-D)
    BDSP_BLE_SLOT_ASSET        = 0x0008,  // Write/Notify, chunked file -> SD card (audio clips)
};

// DeviceInfo capability bits (integer key 2 in the CBOR map).
enum {
    BDSP_CAP_HAS_AUDIO      = 1u << 0,
    BDSP_CAP_HAS_IMU        = 1u << 1,
    BDSP_CAP_HAS_RTC        = 1u << 2,
    BDSP_CAP_HAS_SD         = 1u << 3,
    BDSP_CAP_SPEEDCAM_BUILD = 1u << 4,   // CONFIG_BDSP_FEATURE_SPEEDCAM was set
    BDSP_CAP_HAS_OTA        = 1u << 5,   // dual-slot partition table, OTA characteristic live
};
