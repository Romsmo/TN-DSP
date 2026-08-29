// ble_transport.h — NimBLE GATT peripheral implementing transport::ITransport.
//
// The app writes DisplayState / Settings / Time and subscribes to Event
// notifications. One service, five characteristics (see ble_uuids.h). All wire
// framing is CBOR via core/codec/protocol.
#pragma once

#include <cstdint>
#include <functional>

#include "core/model/settings.h"
#include "transport/transport.h"

namespace transport::ble {

class BleTransport : public transport::ITransport {
public:
    // ── ITransport ─────────────────────────────────────────────────────────
    void start() override;
    void stop() override;
    transport::LinkStatus link() const override;

    bool poll(core::DisplayState& out) override;         // latest State write, if new
    void emit(const core::DeviceEvent& e) override;      // notify on the Event char
    void push_settings(const core::Settings& s) override;
    bool poll_settings(core::Settings& out) override;
    bool state_stale() const override;

    // ── config (call before start()) ───────────────────────────────────────
    void set_caps(uint32_t caps);
    void set_fw_version(const char* v);
    // Fires (from the BLE task) when the app writes the Time characteristic.
    void on_time_set(std::function<void(uint32_t epochS, int16_t tzOffsetMin)> cb);

    // "XX:XX:XX:XX:XX:XX", empty until the host has synced (~1 s after start()).
    const char* address_string() const;
};

}  // namespace transport::ble
