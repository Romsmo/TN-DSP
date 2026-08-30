// core/app/app_runner.h — the shared main-loop body: poll the transport, feed the
// state machine, pump audio, sync + persist settings, hand the view to a
// renderer. Used by both the firmware composition root and the simulator.
#pragma once

#include <cstdint>
#include <functional>

#include "core/audio/audio_service.h"
#include "core/cache/cache_store.h"
#include "core/model/display_state.h"
#include "core/model/settings.h"
#include "core/settings/settings_store.h"
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
    // When set, remote Settings writes and local Settings changes are persisted.
    void set_settings_store(SettingsStore* s) { store_ = s; }
    // When set, the last rendered DisplayState is persisted every ~30 s so it
    // survives an ignition cycle (see CacheStore).
    void set_cache_store(CacheStore* c) { cache_store_ = c; }

    using Renderer = std::function<void(const DisplayState&, const ViewModel&)>;

    // One iteration. Call from the UI/io loop.
    void step(const Renderer& render) {
        DisplayState ds;
        if (transport_.poll(ds)) sm_.on_state(ds);

        // Settings written by the app -> adopt + persist locally.
        Settings incoming;
        if (transport_.poll_settings(incoming)) {
            set_settings(incoming);
            if (store_) store_->save(incoming);
        }
        // Settings changed here -> push to the app + persist.
        if (sm_.settings_dirty()) {
            sm_.clear_settings_dirty();
            transport_.push_settings(sm_.settings());
            if (store_) store_->save(sm_.settings());
        }

        // A connected link that has gone quiet counts as "the app went away":
        // fall back to the cache + the no-app badge until packets resume.
        transport::LinkStatus link = transport_.link();
        if (link == transport::LinkStatus::Connected && transport_.state_stale()) {
            link = transport::LinkStatus::Advertising;
        }
        sm_.on_link(link);

        const uint64_t now = clock_.now_ms();
        sm_.tick(now, epoch_base_ + static_cast<uint32_t>(now / 1000));

        if (audio_) {
            audio_->on_state(sm_.effective_state());
            audio_->pump();
        }

        // Persist the last rendered state periodically (bounded NVS wear).
        if (cache_store_ && sm_.have_live_state() &&
            now - last_cache_ms_ >= kCachePersistMs) {
            last_cache_ms_ = now;
            cache_store_->save(sm_.effective_state());
        }

        render(sm_.effective_state(), sm_.view());
    }

    void set_epoch_base(uint32_t e) { epoch_base_ = e; }

private:
    static constexpr uint64_t kCachePersistMs = 30'000;

    transport::ITransport& transport_;
    hal::Clock&            clock_;
    AppStateMachine        sm_;
    audio::AudioService*   audio_ = nullptr;
    SettingsStore*         store_ = nullptr;
    CacheStore*            cache_store_ = nullptr;
    uint64_t              last_cache_ms_ = 0;
    uint32_t               epoch_base_ = 1'700'000'000;
};

}  // namespace core
