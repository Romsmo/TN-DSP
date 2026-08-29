// transport/mux/mux_transport.h — routes ITransport calls to one of two backends.
//
// The firmware runs the live link (BLE) as `primary` and a scripted `demo`
// backend; a BOOT long-press flips between them at runtime without the app logic
// knowing which is feeding it. Both backends must outlive the mux.
#pragma once

#include "transport/transport.h"

namespace transport::mux {

class MuxTransport : public transport::ITransport {
public:
    MuxTransport(transport::ITransport& primary, transport::ITransport& demo)
        : primary_(primary), demo_(demo) {}

    // Switch the data source. The demo backend is start()/stop()ed on the flip;
    // the primary is left running so a real client can (re)connect underneath.
    void use_demo(bool on) {
        if (on == demo_active_) return;
        demo_active_ = on;
        if (on) demo_.start();
        else demo_.stop();
    }
    bool demo_active() const { return demo_active_; }

    // ── ITransport ─────────────────────────────────────────────────────────
    void start() override { primary_.start(); }
    void stop() override {
        primary_.stop();
        demo_.stop();
        demo_active_ = false;
    }
    transport::LinkStatus link() const override { return active().link(); }
    bool poll(core::DisplayState& out) override { return active().poll(out); }
    void emit(const core::DeviceEvent& e) override { active().emit(e); }
    void push_settings(const core::Settings& s) override { active().push_settings(s); }
    bool poll_settings(core::Settings& out) override { return active().poll_settings(out); }
    bool state_stale() const override { return active().state_stale(); }

private:
    transport::ITransport&       active() { return demo_active_ ? demo_ : primary_; }
    const transport::ITransport& active() const { return demo_active_ ? demo_ : primary_; }

    transport::ITransport& primary_;
    transport::ITransport& demo_;
    bool                   demo_active_ = false;
};

}  // namespace transport::mux
