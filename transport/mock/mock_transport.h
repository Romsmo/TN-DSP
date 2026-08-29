// transport/mock/mock_transport.h — a scripted ITransport for the demo mode and
// the simulator. Plays a timeline of DisplayStates; records received events.
#pragma once

#include <cstdint>
#include <vector>

#include "hal/clock.h"
#include "transport/transport.h"

namespace transport::mock {

struct Step {
    uint32_t          at_ms;   // time offset from script start
    core::DisplayState state;
};

class MockTransport : public transport::ITransport {
public:
    explicit MockTransport(hal::Clock& clock) : clock_(clock) {}

    void load(std::vector<Step> script, bool loop = true);

    // ITransport
    void start() override;
    void stop() override;
    transport::LinkStatus link() const override { return link_; }
    bool poll(core::DisplayState& out) override;
    void emit(const core::DeviceEvent& e) override { events_.push_back(e); }
    void push_settings(const core::Settings& s) override {
        last_settings_ = s;
        ++settings_pushes_;
    }
    bool poll_settings(core::Settings& out) override {
        if (!incoming_settings_new_) return false;
        incoming_settings_new_ = false;
        out = incoming_settings_;
        return true;
    }
    bool state_stale() const override;

    // test / sim introspection
    const std::vector<core::DeviceEvent>& events() const { return events_; }
    void clear_events() { events_.clear(); }
    const core::Settings& pushed_settings() const { return last_settings_; }
    int  settings_pushes() const { return settings_pushes_; }
    void inject_settings(const core::Settings& s) {   // simulate an app-side write
        incoming_settings_ = s;
        incoming_settings_new_ = true;
    }

private:
    hal::Clock&           clock_;
    std::vector<Step>     script_;
    bool                  loop_ = true;
    bool                  running_ = false;
    uint64_t              start_ms_ = 0;
    size_t                next_ = 0;
    uint32_t              seq_ = 0;
    uint64_t              last_delivered_ms_ = 0;
    transport::LinkStatus link_ = transport::LinkStatus::Disconnected;

    std::vector<core::DeviceEvent> events_;
    core::Settings                 last_settings_{};
    int                            settings_pushes_ = 0;
    core::Settings                 incoming_settings_{};
    bool                           incoming_settings_new_ = false;
};

// A ready-made demo timeline: home -> hazard approach with countdown -> confirm
// prompt -> clear -> loop. Used by BOOT-long-press demo mode and the simulator.
std::vector<Step> demo_script();

}  // namespace transport::mock
