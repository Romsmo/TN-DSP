#include "transport/mock/mock_transport.h"

#include "core/model/sign_list.h"

namespace transport::mock {

using core::DisplayState;
using core::HazardType;

void MockTransport::load(std::vector<Step> script, bool loop) {
    script_ = std::move(script);
    loop_ = loop;
    next_ = 0;
}

void MockTransport::start() {
    running_ = true;
    link_ = transport::LinkStatus::Connected;
    start_ms_ = clock_.now_ms();
    last_delivered_ms_ = start_ms_;
    next_ = 0;
}

void MockTransport::stop() {
    running_ = false;
    link_ = transport::LinkStatus::Disconnected;
}

bool MockTransport::poll(DisplayState& out) {
    if (!running_ || script_.empty()) return false;

    const uint64_t now = clock_.now_ms();
    const uint64_t elapsed = now - start_ms_;

    bool delivered = false;
    while (next_ < script_.size() && elapsed >= script_[next_].at_ms) {
        out = script_[next_].state;
        ++next_;
        delivered = true;
    }
    if (delivered) {
        last_delivered_ms_ = now;
        return true;
    }

    if (next_ >= script_.size() && loop_) {
        start_ms_ = now;
        next_ = 0;
    }
    return false;
}

bool MockTransport::state_stale() const {
    return running_ && (clock_.now_ms() - last_delivered_ms_) > 8000;
}

std::vector<Step> demo_script() {
    auto base = []() {
        DisplayState s;
        s.internetConnected = true;
        s.networkPeers = 480000;
        s.speedLimit = 60;
        s.directionArrows = true;
        s.leftSigns.add(core::sign_code(276));
        s.rightSigns.add(core::sign_code(142, 10));
        return s;
    };

    std::vector<Step> v;
    uint32_t seq = 1;
    auto add = [&](uint32_t at, DisplayState s) {
        s.stateSeq = seq++;
        s.epochS = 1'700'000'000 + at / 1000;
        v.push_back({at, s});
    };

    add(0, base());

    uint32_t cue_seq = 1;
    for (int d = 800; d >= 100; d -= 100) {                 // hazard approaches
        DisplayState s = base();
        s.directionArrows = false;
        s.hazardActive = true;
        s.hazardType = HazardType::MobileSpeedCamera;
        s.hazardDistanceM = d;
        s.hazardSpeedLimit = 50;
        if (d == 800 || d == 300) {                          // announce twice
            core::AudioCue c;
            c.seq = cue_seq++;
            c.mode = core::AudioMode::Voice;
            c.event = d == 300 ? core::AudioEvent::HazardImminent
                               : core::AudioEvent::HazardApproach;
            c.hazardType = HazardType::MobileSpeedCamera;
            c.distanceM = d;
            c.speedValue = 50;
            s.audioCue = c;
        }
        add(static_cast<uint32_t>(4000 + (800 - d) * 6), s);
    }

    {                                                        // confirm prompt
        DisplayState s = base();
        s.hazardActive = false;
        s.confirmPromptActive = true;
        s.confirmPromptTimeoutS = 15;
        core::AudioCue c;
        c.seq = cue_seq++;
        c.mode = core::AudioMode::Tones;
        c.event = core::AudioEvent::ConfirmPrompt;
        s.audioCue = c;
        add(9200, s);
    }

    add(13000, base());                                      // back to normal

    return v;
}

}  // namespace transport::mock
