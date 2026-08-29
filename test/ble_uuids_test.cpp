// Guards the provisional BLE UUID scheme: byte layout and slot encoding.
#include "doctest/doctest.h"

#include <array>
#include <cstdint>

#include "firmware/components/transport_ble/ble_uuids.h"

TEST_CASE("BLE UUID128 macro encodes the slot little-endian into the 128-bit base") {
    std::array<uint8_t, 16> svc  = BDSP_BLE_UUID128(BDSP_BLE_SLOT_SERVICE);
    std::array<uint8_t, 16> state = BDSP_BLE_UUID128(BDSP_BLE_SLOT_STATE);
    std::array<uint8_t, 16> sett  = BDSP_BLE_UUID128(0x0004);

    // Base tail (079a....-d673-469c-98a0-12c0c5eb43b3) stored little-endian.
    CHECK(svc[0] == 0xb3);
    CHECK(svc[15] == 0x07);
    CHECK(svc[14] == 0x9a);

    // Slot bytes at indices 12 (low) / 13 (high).
    CHECK(svc[12] == 0x00);
    CHECK(svc[13] == 0x00);
    CHECK(state[12] == 0x01);
    CHECK(state[13] == 0x00);
    CHECK(sett[12] == 0x04);

    // Only the slot bytes differ between characteristics.
    for (std::size_t i = 0; i < svc.size(); ++i) {
        if (i == 12 || i == 13) continue;
        CHECK(svc[i] == state[i]);
    }
}

TEST_CASE("Capability bits are distinct powers of two") {
    const unsigned all = BDSP_CAP_HAS_AUDIO | BDSP_CAP_HAS_IMU | BDSP_CAP_HAS_RTC |
                         BDSP_CAP_HAS_SD | BDSP_CAP_SPEEDCAM_BUILD;
    CHECK(all == 0x1F);
}
