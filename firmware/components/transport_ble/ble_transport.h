// ble_transport.h — NimBLE GATT peripheral implementing transport::ITransport.
#pragma once

#include "core/model/settings.h"
#include "transport/transport.h"

namespace transport::ble {

class BleTransport : public transport::ITransport {
public:
    void start() override;
    void stop() override;
    transport::LinkStatus link() const override { return link_; }

    bool poll(core::DisplayState& out) override;         // latest State write, if new
    void emit(const core::DeviceEvent& e) override;      // notify on Event char
    void push_settings(const core::Settings& s) override;
    bool poll_settings(core::Settings& out) override;
    bool state_stale() const override;

    // Capability bitmask advertised in DeviceInfo.
    void set_caps(uint32_t caps) { caps_ = caps; }

private:
    transport::LinkStatus link_ = transport::LinkStatus::Disconnected;
    uint32_t              caps_ = 0;
};

}  // namespace transport::ble
