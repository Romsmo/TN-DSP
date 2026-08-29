# Architecture

## Guiding principle (binding)

**The display renders; it never computes.**

The board draws everything locally — sign, icons, animations, navigation — but contains **no logic
that computes or interprets anything**: no map-matching, no distance calculation, no decision about
*what* should be shown. Over BLE it receives finished values (`DisplayState`) and only paints them.
In the other direction it emits raw events (`ReportSubmitted`, `ConfirmResponse`, …); what the app
does with them is entirely the app's concern.

Concretely, the firmware **must not**:

- derive `hazardActive`, `confirmPromptActive`, distances, or ETA from anything;
- decide which speed-limit sign is "current" beyond the explicit `speedLimit` / `hazardSpeedLimit`
  fields;
- filter warnings by GPS position or road geometry.

The firmware **may**:

- run pure UI-state logic: which screen/overlay is visible, gesture math, timeouts, animations;
- apply the user's category filter as a *display* gate (see below) — this is suppression of a
  received overlay, not computation of a warning;
- convert the report-slider value between km/h and mph for display (canonical wire unit is km/h);
- abbreviate `networkPeers` for display ("480000" → "480K").

The one grey area is **audio cue playback**: the app sends a structured `AudioCue` (it decides the
"when"); the board composes the phrase / picks the tone and plays it (the "how it sounds"). The
board does not decide *whether* to announce.

## Layers

```
                 ┌────────────────────────────────────────────┐
   app/ ───────► │ composition root: wires HAL + core + UI +   │
                 │ transport, starts tasks. Only place with    │
                 │ knowledge of all layers. No global state.   │
                 └───────────────┬────────────────────────────┘
                                 │
        ┌────────────────────────┼────────────────────────┐
        ▼                        ▼                        ▼
   ┌─────────┐            ┌──────────────┐          ┌───────────┐
   │  ui/    │◄── state ──│    core/     │── evt ──►│ transport/│
   │ (LVGL)  │── evt ────►│ model,state, │◄─ state ─│ ble|mock| │
   │         │            │ codec,cache, │          │ serial    │
   └────┬────┘            │ settings,…   │          └─────┬─────┘
        │                 └──────┬───────┘                │
        │ port-lock              │ interfaces             │
        ▼                        ▼                        ▼
   ┌────────────────────────  hal/  ───────────────────────────┐
   │ DisplayDriver TouchDriver ImuDriver RtcDriver PowerMonitor │
   │ Nvs AudioDriver Storage Buttons Clock                      │
   └───────────────────────────────────────────────────────────┘
             concrete impls: firmware/components/hal_esp/  (target)
                             sim/  + test/ fakes            (host)
```

### `core/` and `ui/` — platform-independent

Contain **zero** ESP-IDF / Arduino includes. Everything platform-specific is reached through
`hal/` interfaces. This is exactly what makes the host test build and the LVGL-SDL simulator
possible — they compile the *same* `core/` and `ui/` sources against host fakes.

### One data-flow interface

All inbound data flows through **one** interface pair:

```cpp
struct IStateSource { virtual bool poll(DisplayState& out) = 0; };   // returns true on new state
struct IEventSink   { virtual void emit(const DeviceEvent& e) = 0; };
```

`BleTransport`, `MockTransport`, `SerialTransport` are interchangeable implementations. The choice
is made in **one** place in the composition root: a compile flag picks the default, and a runtime
fallback switches to `MockTransport` when no BLE client is present (see *No-app behaviour*).

### Tasks

Two FreeRTOS tasks:

| Task | Core | Responsibility |
|---|---|---|
| `ui_task` | (C6 is single-core) | LVGL tick + render + input; owns the LVGL port lock |
| `io_task` | ″ | transport (BLE/mock), HAL polling (touch, buttons, IMU), audio pump |

Communication is **only** through thread-safe queues (`core/bus/`). LVGL is touched **only** under
`lvgl_port_lock()`. No allocation in the render path. The design maps 1:1 onto a dual-core SoC
(e.g. an S3) by pinning the two tasks to different cores — nothing else changes.

## Extensibility rules

- **New warning / report category** = one row in the category table
  (`core/catalog/category_registry`): `{id, label-key, icon, group, needsSpeed, speedcamFlag}`.
  No existing screen is touched.
- **New screen** = one `ScreenManager` registry entry. No existing screen is touched.
- **New protocol field** = new CBOR integer key. Old firmware ignores unknown keys; it never
  rejects the packet. `HazardType` values are frozen (0–10); new hazard types are appended and
  render as a generic icon on old firmware.

## Category filter (second-layer suppression)

The companion app owns the per-category "warn me about X" toggles (including general hazards like
*Stau*). The flags are synced over the **Settings** characteristic (bidirectional) and persisted
to NVS. The firmware also enforces them: if a `DisplayState` arrives with `hazardActive` for a
disabled `hazardType`, the hazard overlay is suppressed and the event logged. This is belt-and-
suspenders on top of the app's own filtering — the app is still the primary filter.

Independently, `CONFIG_FEATURE_SPEEDCAM` (Kconfig, compile time) removes the speed-camera enum
range, icons and report column entirely, for builds intended to comply with §23 (1b) StVO. The
`DeviceInfo` capability bitmask advertises whether speed-camera support was compiled in.

## No-app behaviour

| Situation | Behaviour |
|---|---|
| First boot / never bonded | "Handy verbinden" screen; Settings still reachable by swipe |
| Bonded but disconnected | Home from `OfflineCache` + subtle "no app" badge; "Handy verbinden" via Settings; cache greys out after a configurable age (default 10 min) |
| BOOT long-press (1.5 s) | Toggle forced **demo mode**: `MockTransport` plays scripted scenarios so every screen can be walked on the bench |
| No `DisplayState` for N s while "connected" | Connection watchdog → treat as disconnected |

## Theme

**Single dark theme, always.** The AMOLED runs black (`#000000`) with high-contrast content.
There is no day/night switching, no "Tag/Nacht" button in Settings, and no RTC-based theme
fallback. The sketch's light background is intentionally overridden.

`nightMode` in `DisplayState` is kept for protocol stability; the board uses it only as an
optional cue to reduce panel brightness. `Settings::theme` / `SettingsThemeChanged` are retained
in the wire format but currently ignored by the firmware.
