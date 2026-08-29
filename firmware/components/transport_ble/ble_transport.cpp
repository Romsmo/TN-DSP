// ble_transport.cpp — NimBLE GATT peripheral.
//
// M7 status: structure + the codec wiring. The NimBLE host callbacks are sketched
// against the ESP-IDF `esp-nimble` API; fill in once ESP-IDF is in the loop.
#include "firmware/components/transport_ble/ble_transport.h"

#include <cstring>

#include "core/codec/protocol.h"
#include "firmware/components/transport_ble/ble_uuids.h"

// TODO(M7): #include "nimble/nimble_port.h" / "host/ble_hs.h" / "host/ble_gatt.h" ...

namespace transport::ble {

namespace {

// Latest State write from the app, handed to poll().
core::DisplayState g_pending_state{};
bool              g_state_new = false;
uint32_t         g_last_seq = 0;
uint64_t         g_last_state_rx_ms = 0;

core::Settings g_settings{};
bool           g_settings_new = false;

// ── GATT characteristic write handlers ─────────────────────────────────────
// int on_state_write(uint16_t conn, const uint8_t* buf, uint16_t len):
//     core::DisplayState s;
//     if (!core::proto::decode_state(buf, len, s)) return BLE_ATT_ERR_UNLIKELY;
//     if (s.stateSeq != 0 && s.stateSeq <= g_last_seq) return 0;   // stale
//     g_last_seq = s.stateSeq; g_pending_state = s; g_state_new = true;
//     g_last_state_rx_ms = now_ms();
//     return 0;
//
// int on_settings_write(...):  core::proto::decode_settings(...) -> g_settings, g_settings_new = true
// int on_time_write(...):      core::proto::decode_time(...) -> rtc.set(...)
// int on_info_read(ctxt):      core::proto::encode_device_info({fw, caps, 466, 466}, out, cap)
// int on_settings_read(ctxt):  core::proto::encode_settings(g_settings, out, cap)

const struct {
    uint8_t service[16] = BDSP_BLE_UUID128(BDSP_BLE_SLOT_SERVICE);
    uint8_t state[16]   = BDSP_BLE_UUID128(BDSP_BLE_SLOT_STATE);
    uint8_t event[16]   = BDSP_BLE_UUID128(BDSP_BLE_SLOT_EVENT);
    uint8_t info[16]    = BDSP_BLE_UUID128(BDSP_BLE_SLOT_DEVICE_INFO);
    uint8_t settings[16] = BDSP_BLE_UUID128(BDSP_BLE_SLOT_SETTINGS);
    uint8_t time[16]    = BDSP_BLE_UUID128(BDSP_BLE_SLOT_TIME);
} kUuids;

}  // namespace

void BleTransport::start() {
    // nimble_port_init(); ble_svc_gap_device_name_set("BlitzerdeDsp");
    // register the GATT table above; ble_gap_adv_start() with the service UUID.
    link_ = transport::LinkStatus::Advertising;
    (void)kUuids;
}

void BleTransport::stop() {
    // ble_gap_adv_stop(); nimble_port_stop();
    link_ = transport::LinkStatus::Disconnected;
}

bool BleTransport::poll(core::DisplayState& out) {
    if (!g_state_new) return false;
    g_state_new = false;
    out = g_pending_state;
    return true;
}

void BleTransport::emit(const core::DeviceEvent& e) {
    uint8_t buf[core::proto::kMaxPayload];
    const size_t n = core::proto::encode_event(e, buf, sizeof(buf));
    if (n == 0) return;
    // ble_gatts_notify_custom(conn_handle_, event_val_handle_, om_from(buf, n));
    (void)n;
}

void BleTransport::push_settings(const core::Settings& s) {
    g_settings = s;
    // encode + ble_gatts_notify_custom on the Settings characteristic
}

bool BleTransport::poll_settings(core::Settings& out) {
    if (!g_settings_new) return false;
    g_settings_new = false;
    out = g_settings;
    return true;
}

bool BleTransport::state_stale() const {
    // return link_ == Connected && (now_ms() - g_last_state_rx_ms) > CONFIG_BDSP_STATE_WATCHDOG_S*1000;
    return false;
}

}  // namespace transport::ble
