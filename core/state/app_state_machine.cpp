#include "core/state/app_state_machine.h"

#include "core/state/warning_filter_apply.h"

namespace core {

namespace {
constexpr uint64_t kMs = 1;
}

void AppStateMachine::on_link(transport::LinkStatus link) {
    link_ = link;
    if (link == transport::LinkStatus::Connected) {
        connect_requested_ = false;
        bonded_ = true;
    }
}

void AppStateMachine::on_state(const DisplayState& s) {
    // Drop stale packets.
    if (have_live_ && s.stateSeq != 0 && s.stateSeq <= effective_.stateSeq &&
        effective_.stateSeq != 0) {
        return;
    }

    DisplayState filtered = s;
    apply_filter(filtered, settings_.filter);

    effective_ = filtered;
    have_live_ = true;
    cache_.put(filtered);
}

void AppStateMachine::open_melden() {
    nav_ = Screen::Melden;
    melden_deadline_ms_ = 0;  // set on next tick
    sink_.emit(DeviceEvent{ReportPressed{}});
}

void AppStateMachine::open_settings() { nav_ = Screen::Settings; }

void AppStateMachine::go_home() { nav_ = Screen::Home; }

void AppStateMachine::melden_activity() { melden_deadline_ms_ = 0; /* re-armed on tick */ }

void AppStateMachine::submit_report(HazardType type, std::optional<int> speed_kmh) {
    ReportSubmitted ev;
    ev.type = type;
    ev.speedKmh = speed_kmh;
    sink_.emit(DeviceEvent{ev});
    nav_ = Screen::Home;
}

void AppStateMachine::answer_confirm(bool seen) {
    if (!confirm_showing_) return;
    confirm_showing_ = false;
    confirm_handled_ = true;
    confirm_deadline_ms_ = 0;
    sink_.emit(DeviceEvent{ConfirmResponse{seen}});
}

void AppStateMachine::request_connect() {
    connect_requested_ = true;
    sink_.emit(DeviceEvent{SettingsPhoneConnect{}});
}

void AppStateMachine::set_sound(bool on) {
    settings_.soundEnabled = on;
    sink_.emit(DeviceEvent{SettingsSoundToggle{on}});
}

void AppStateMachine::set_units(bool use_mph) {
    settings_.useMph = use_mph;
    sink_.emit(DeviceEvent{SettingsUnitChanged{use_mph}});
}

void AppStateMachine::toggle_demo() { demo_ = !demo_; }

void AppStateMachine::tick(uint64_t now_ms, uint32_t now_epoch_s) {
    if (start_epoch_s_ == 0 && now_epoch_s != 0) start_epoch_s_ = now_epoch_s;

    // Melden idle timeout.
    if (nav_ == Screen::Melden) {
        if (melden_deadline_ms_ == 0) melden_deadline_ms_ = now_ms + kMeldenTimeoutMs * kMs;
        else if (now_ms >= melden_deadline_ms_) nav_ = Screen::Home;
    } else {
        melden_deadline_ms_ = 0;
    }

    // Confirm prompt lifetime (edge-triggered, latched so it doesn't re-arm after
    // the user answers / it times out while the app still has the flag set).
    const bool want_confirm = effective_.confirmPromptActive;
    if (!want_confirm) {
        confirm_showing_ = false;
        confirm_handled_ = false;               // ready for the next prompt
        confirm_deadline_ms_ = 0;
    } else if (!confirm_showing_ && !confirm_handled_) {
        confirm_showing_ = true;
        const int secs =
            effective_.confirmPromptTimeoutS > 0 ? effective_.confirmPromptTimeoutS : 15;
        confirm_deadline_ms_ = now_ms + static_cast<uint64_t>(secs) * 1000;
    }
    if (confirm_showing_ && now_ms >= confirm_deadline_ms_) {
        confirm_showing_ = false;               // silent timeout (no event)
        confirm_handled_ = true;
        confirm_deadline_ms_ = 0;
    }

    // Heartbeat.
    if (now_ms >= next_heartbeat_ms_) {
        next_heartbeat_ms_ = now_ms + kHeartbeatMs * kMs;
        const uint32_t uptime = (start_epoch_s_ && now_epoch_s >= start_epoch_s_)
                                    ? now_epoch_s - start_epoch_s_
                                    : 0;
        sink_.emit(DeviceEvent{Heartbeat{uptime}});
    }

    recompute(now_ms, now_epoch_s);
}

void AppStateMachine::recompute(uint64_t now_ms, uint32_t now_epoch_s) {
    const bool connected = link_ == transport::LinkStatus::Connected;

    // Effective state: live when connected, else cached.
    if (!connected && cache_.has()) {
        effective_ = cache_.get();
    }

    ViewModel vm;
    vm.demo = demo_;
    vm.linkDown = !connected && !demo_;
    vm.cacheStale = vm.linkDown && cache_.has() &&
                    cache_.is_stale(now_epoch_s, settings_.cacheStaleMinutes);

    // Screen.
    const bool need_connect = (!bonded_ && !demo_) || connect_requested_;
    if (need_connect) {
        vm.screen = Screen::Connect;
    } else {
        vm.screen = nav_;
    }

    // Overlay priority: confirm > hazard.
    if (confirm_showing_) {
        vm.overlay = OverlayKind::Confirm;
        vm.navLocked = true;
        if (now_ms < confirm_deadline_ms_) {
            vm.confirmSecondsLeft =
                static_cast<uint8_t>((confirm_deadline_ms_ - now_ms + 999) / 1000);
        }
    } else if (effective_.hazardActive) {
        vm.overlay = OverlayKind::Hazard;
    }

    vm_ = vm;
}

}  // namespace core
