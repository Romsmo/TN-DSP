// sim/scenarios.h — canned DisplayStates for driving the UI from the keyboard.
#pragma once

#include "core/model/display_state.h"

namespace sim {

struct Scenario {
    const char*        key;    // single keyboard key, e.g. "1"
    const char*        name;   // screenshot filename / log label
    core::DisplayState state;
};

int             scenario_count();
const Scenario& scenario_at(int i);
const Scenario* scenario_for_key(char key);

}  // namespace sim
