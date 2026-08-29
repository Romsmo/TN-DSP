// transport/transport.h — the ONE data-flow boundary.
//
// Inbound: IStateSource yields a fresh DisplayState. Outbound: IEventSink takes a
// DeviceEvent. BLE / Mock / Serial are interchangeable implementations of ITransport;
// the choice is made in one place in the composition root.
#pragma once

#include "core/model/device_event.h"
#include "core/model/display_state.h"
#include "core/model/settings.h"

namespace transport {

struct IStateSource {
    virtual ~IStateSource() = default;
    // Fills `out` and returns true when a newer state is available (seq advanced).
    virtual bool poll(core::DisplayState& out) = 0;
};

struct IEventSink {
    virtual ~IEventSink() = default;
    virtual void emit(const core::DeviceEvent& e) = 0;
};

enum class LinkStatus { Disconnected, Advertising, Connected };

struct ITransport : IStateSource, IEventSink {
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual LinkStatus link() const = 0;

    // Settings sync is bidirectional. The transport owns the wire format; the app
    // logic hands it the current Settings and gets notified of remote changes.
    virtual void push_settings(const core::Settings& s) = 0;
    virtual bool poll_settings(core::Settings& out) = 0;

    // True once no DisplayState has arrived for the watchdog interval.
    virtual bool state_stale() const = 0;
};

}  // namespace transport
