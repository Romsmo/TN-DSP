// core/state/app_state_machine.h — the platform-independent brain.
//
// Consumes DisplayState packets + user intents + a monotonic clock, produces a
// ViewModel for the UI and emits DeviceEvents. Owns ALL timeout logic (Melden 15 s,
// confirm countdown, heartbeat). No LVGL, no HAL beyond IEventSink.
#pragma once

#include <cstdint>
#include <optional>

#include "core/cache/offline_cache.h"
#include "core/model/device_event.h"
#include "core/model/display_state.h"
#include "core/model/hazard_type.h"
#include "core/model/settings.h"
#include "core/model/ui_config.h"
#include "core/state/view_model.h"
#include "transport/transport.h"

namespace core {

class AppStateMachine {
public:
    explicit AppStateMachine(transport::IEventSink& sink) : sink_(sink) {}

    // ── configuration ──────────────────────────────────────────────────────
    void set_settings(const Settings& s) { settings_ = s; }
    void set_ui_config(const UiConfig& c) { ui_ = c; }
    const UiConfig& ui_config() const { return ui_; }
    void set_bonded(bool b) { bonded_ = b; }

    // Seed the offline cache from persistent storage on boot: the board shows
    // the last *sign* context (greyed once stale) on Home instead of the pairing
    // screen, until the app reconnects. Momentary alerts (hazard, confirm) are
    // dropped — the app re-sends them if they still apply. Implies we were paired.
    void prime_cache(DisplayState s) {
        s.hazardActive = false;
        s.hazardType.reset();
        s.hazardDistanceM.reset();
        s.hazardSpeedLimit.reset();
        s.confirmPromptActive = false;
        s.audioCue.reset();
        cache_.put(s);
        effective_ = s;
        bonded_ = true;
    }
    bool have_live_state() const { return have_live_; }

    // ── inbound ────────────────────────────────────────────────────────────
    void on_state(const DisplayState& s);              // from the transport
    void on_link(transport::LinkStatus link);          // connection status
    void tick(uint64_t now_ms, uint32_t now_epoch_s);  // drives timeouts

    // ── user intents (from ui/) ────────────────────────────────────────────
    void open_melden();          // "+" or swipe-left
    void open_settings();        // swipe-up
    void go_home();              // back swipe / completed input
    void melden_activity();      // any interaction on Melden -> resets its timeout
    void submit_report(HazardType type, std::optional<int> speed_kmh);
    void answer_confirm(bool seen);
    void request_connect();      // "Handy verbinden" pressed
    void set_sound(bool on);
    void set_units(bool use_mph);
    void toggle_demo();

    // ── outbound ───────────────────────────────────────────────────────────
    // Offline / Idle / Driving — derived from the link, whether a live State has arrived
    // since the connect, and its driveActive flag. Demo mode always drives.
    Presence            presence() const;
    const ViewModel&    view() const { return vm_; }
    const DisplayState& effective_state() const { return effective_; }
    bool                demo() const { return demo_; }

    // The local Settings. `settings_dirty()` is set when the user changed one
    // here (so the runner can persist + push it) and cleared by the runner.
    const Settings& settings() const { return settings_; }
    bool settings_dirty() const { return settings_dirty_; }
    void clear_settings_dirty() { settings_dirty_ = false; }

    static constexpr uint32_t kMeldenTimeoutMs = 15000;
    static constexpr uint32_t kHeartbeatMs     = 10000;

private:
    void recompute(uint64_t now_ms, uint32_t now_epoch_s);

    transport::IEventSink& sink_;
    Settings               settings_{};
    UiConfig               ui_{};
    OfflineCache           cache_;

    DisplayState effective_{};
    bool         settings_dirty_ = false;
    bool         have_live_ = false;
    bool         bonded_ = false;
    bool         demo_ = false;
    bool         connect_requested_ = false;
    transport::LinkStatus link_ = transport::LinkStatus::Disconnected;

    Screen   nav_ = Screen::Home;             // where the user navigated
    uint64_t melden_deadline_ms_ = 0;
    uint64_t confirm_deadline_ms_ = 0;
    bool     confirm_showing_ = false;        // our own copy of the prompt lifetime
    bool     confirm_handled_ = false;        // answered/timed-out this prompt instance
    uint64_t next_heartbeat_ms_ = kHeartbeatMs;
    uint32_t start_epoch_s_ = 0;

    ViewModel vm_{};
};

}  // namespace core
