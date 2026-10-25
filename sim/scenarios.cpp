#include "sim/scenarios.h"

#include <array>

using core::AudioCue;
using core::AudioEvent;
using core::AudioMode;
using core::DisplayState;
using core::HazardType;
using core::SpeedUnit;

namespace sim {
namespace {

DisplayState home() {
    DisplayState s;
    s.internetConnected = true;
    s.networkPeers = 480000;
    s.speedLimit = 60;
    s.directionArrows = true;
    s.leftSigns.add(core::sign_code(276));      // Überholverbot
    s.rightSigns.add(core::sign_code(142, 10)); // Wildwechsel
    s.rightSigns.add(core::sign_code(110, 4));  // Steigung
    s.stateSeq = 1;
    return s;
}

DisplayState no_internet() {
    DisplayState s = home();
    s.internetConnected = false;
    s.networkPeers = 0;
    s.directionArrows = true;
    s.stateSeq = 2;
    return s;
}

DisplayState hazard() {
    DisplayState s = home();
    s.directionArrows = false;
    s.hazardActive = true;
    s.hazardType = HazardType::MobileSpeedCamera;
    s.hazardDistanceM = 600;
    s.hazardSpeedLimit = 50;
    s.stateSeq = 3;
    s.audioCue = AudioCue{};
    s.audioCue->seq = 1;
    s.audioCue->mode = AudioMode::Voice;
    s.audioCue->event = AudioEvent::HazardApproach;
    s.audioCue->hazardType = HazardType::MobileSpeedCamera;
    s.audioCue->distanceM = 600;
    s.audioCue->speedValue = 50;
    s.audioCue->speedUnit = SpeedUnit::Kmh;
    return s;
}

DisplayState confirm() {
    DisplayState s = home();
    s.confirmPromptActive = true;
    s.confirmPromptTimeoutS = 15;
    s.stateSeq = 4;
    return s;
}

DisplayState reporting() {
    DisplayState s = home();
    s.stateSeq = 5;
    return s;
}

DisplayState settings_scr() {
    DisplayState s = home();
    s.stateSeq = 6;
    return s;
}

DisplayState idle() {            // app connected, no trip running
    DisplayState s = home();
    s.driveActive = false;
    s.stateSeq = 7;
    return s;
}

DisplayState connect() {
    DisplayState s;
    s.internetConnected = false;
    s.stateSeq = 0;
    return s;
}

const std::array<Scenario, 9> kScenarios{{
    {"1", "home", Nav::Home, home()},
    {"2", "no-internet", Nav::Home, no_internet()},
    {"3", "hazard", Nav::Home, hazard()},
    {"4", "melden", Nav::Melden, reporting()},
    {"5", "confirm", Nav::Home, confirm()},
    {"6", "settings", Nav::Settings, settings_scr()},
    {"7", "connect", Nav::ConnectOverlay, connect()},
    {"8", "idle", Nav::Home, idle()},
    {"9", "offline", Nav::Offline, home()},
}};

}  // namespace

int             scenario_count() { return static_cast<int>(kScenarios.size()); }
const Scenario& scenario_at(int i) { return kScenarios[static_cast<size_t>(i)]; }

const Scenario* scenario_for_key(char key) {
    for (const auto& sc : kScenarios) {
        if (sc.key[0] == key) return &sc;
    }
    return nullptr;
}

}  // namespace sim
