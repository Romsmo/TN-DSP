// sim/scenarios.h — canned DisplayStates + a nav target for driving the UI.
#pragma once

#include "core/model/display_state.h"

namespace sim {

enum class Nav { Home, Melden, Settings, ConnectOverlay, Offline };   // Offline: bonded, link down

struct Scenario {
    const char*        key;    // single keyboard key, e.g. "1"
    const char*        name;   // screenshot filename / log label
    Nav                nav;
    core::DisplayState state;
};

int             scenario_count();
const Scenario& scenario_at(int i);
const Scenario* scenario_for_key(char key);

}  // namespace sim
