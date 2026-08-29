// core/app/app_runner.h — the shared main-loop body: poll the transport, feed the
// state machine, pump audio, hand the view to a renderer. Used by both the
// firmware composition root and the simulator.
#pragma once

#include <cstdint>
#include <functional>

#include "core/audio/audio_service.h"
#include "core/model/display_state.h"
#include "core/model/settings.h"
#include "core/state/app_state_machine.h"
#include "core/state/view_model.h"
#include "hal/clock.h"
#include "transport/transport.h"

namespace core {

class AppRunner {
public:
    AppRunner(transport::ITransport& transport, hal::Clock& clock)
        : transport_(transport), clock_(clock), sm_(transport) {}

    AppStateMachine& sm() { return sm_; }
    void set_audio(audio::AudioService* a) { audio_ = a; }
    void set_settings(const Settings& s) {
        sm_.set_settings(s);
        if (audio_) audio_->apply_settings(s);
    }

    using Renderer = std::function<void(const DisplayState&, const ViewModel&)>;

    // One iteration. Call from the UI/io loop.
    void step(const Renderer& render) {
        DisplayState ds;
        if (transport_.poll(ds)) sm_.on_state(ds);
        sm_.on_link(transport_.link());

        const uint64_t now = clock_.now_ms();
        sm_.tick(now, epoch_base_ + static_cast<uint32_t>(now / 1000));

        if (audio_) {
            audio_->on_state(sm_.effective_state());
            audio_->pump();
        }
        render(sm_.effective_state(), sm_.view());
    }

    void set_epoch_base(uint32_t e) { epoch_base_ = e; }

private:
    transport::ITransport& transport_;
    hal::Clock&            clock_;
    AppStateMachine        sm_;
    audio::AudioService*   audio_ = nullptr;
    uint32_t               epoch_base_ = 1'700'000'000;
};

}  // namespace core
