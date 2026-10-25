# App integration — handover to the app side

**Contract revision: `1.1.0`** (protocol version 1), tagged `protocol-v1.1.0` (1.0.0 → 1.1.0 added
State key 14 `driveActive`, additive). Firmware releases: `fw-v0.1.0` (contract 1.0.0) and `fw-v0.2.0`
(contract 1.1.0) — see `release-format.md`.
German summary for the maintainer: [`app-integration.de.md`](app-integration.de.md).

## Where everything is

| Need | File |
|---|---|
| The specification | [`ble-protocol.md`](ble-protocol.md) |
| **Every display function as a tick-off list** | [`app-feature-checklist.md`](app-feature-checklist.md) |
| Who decides what | [`contract/responsibilities.md`](contract/responsibilities.md) |
| Golden vectors, categories, constants | [`contract/vectors/`](contract/vectors/) · overview [`contract/README.md`](contract/README.md) |
| Sign placement + replacement rules | [`contract/sign-rules.json`](contract/sign-rules.json) + `contract/vectors/sign-rules.json` |
| Tested Dart implementation (no Flutter, no Bluetooth) | [`../protocol/dart/`](../protocol/dart/) — package `tn_display_protocol` |
| Update flow | [`release-format.md`](release-format.md), [`compatibility.md`](compatibility.md), [`update-authenticity.md`](update-authenticity.md) |
| Proposals awaiting a decision | [`contract/uuid-proposal.md`](contract/uuid-proposal.md), [`contract/melden-lock-proposal.md`](contract/melden-lock-proposal.md) |

## What the app must do

1. Scan for the service UUID, connect, let the OS negotiate the MTU, read **DeviceInfo**, check the
   protocol version and the capability bits.
2. Subscribe to Event / Settings / UiConfig; write **Time**; write the complete **Settings** set;
   then stream **State** with `stateSeq` starting at **1** on every connection.
3. Send a State **on every change** and at least **every 5 s** as a keep-alive; steady rate ≤ 2/s.
   Also while connected but **not on a trip**: send `driveActive = false` (State key 14) so the display
   shows its logo instead of going dark after 8 s.
4. Keep an `audioCue` (same `seq`) in following States for ≥ 1 s; keep `confirmPromptActive` set for
   the whole timeout (the device may drop superseded packets).
5. Own the decisions the display does not make: sign columns and replacement ([`contract/sign-rules.json`](contract/sign-rules.json)),
   the hazard filter (`filterBits`), when to announce, volume, units, night flag, when to show the
   "Gesehen?" prompt.
6. Handle device events: `ReportSubmitted`, `ConfirmResponse`, settings changes made on the device
   (Settings notify), `Heartbeat`.
7. Reconnect after any drop and start again at step 1 — **restart `stateSeq` at 1**.
8. Offer updates only when `hasOta` is set; follow the update flow in `release-format.md`.

## Capability bits (DeviceInfo key 2)

| Bit | Name | If clear, the app … |
|---|---|---|
| 0 | hasAudio | does not send cues, hides sound settings |
| 1 | hasImu | (unused today) |
| 2 | hasRtc | still writes Time, does not rely on the clock surviving power loss |
| 3 | hasSD | does not offer voice mode or asset upload |
| 4 | speedcamBuild | hides hazard categories 0–4 and never sends them |
| 5 | hasOta | does not offer firmware updates |

## Using the Dart package

```dart
import 'package:tn_display_protocol/tn_display_protocol.dart';

final bytes = const DisplayState(
  internetConnected: true, speedLimit: 60, directionArrows: true, stateSeq: 1,
).encode();                       // write to uuidForSlot(Slot.state), without response

var signs = const SignLists.empty();
signs = addSign(signs, '276').lists;           // goes left; replaces its family
final state = DisplayState(leftSigns: signs.leftCodes, rightSigns: signs.rightCodes, stateSeq: 2);
```

Everything is pinned by `protocol/dart/test/` against the golden vectors (also run in CI).
The only third-party dependency is `crypto` (BSD-3-Clause, dart.dev) for SHA-256; the CBOR codec is
the package's own (the device uses a matching minimal codec), so no CBOR licence is involved.

## Known limitations

- **Security level is open today**: no characteristic requires pairing or encryption, so OTA and
  Asset can be used by any central in range. This is documented as a decision, not a feature
  ([`ble-protocol.md`](ble-protocol.md) *Security level*, [`update-authenticity.md`](update-authenticity.md)).
- **Memory is tight** on the device (~41 KB free internal RAM in steady state): the protocol keeps
  payloads small; do not expect large additions without a firmware release.
- **Latest wins** for State and a send-rate recommendation instead of a hard limit (see Delivery
  semantics).
- A fast **live swipe** between Home and Melden can show a brief tearing seam (display hardware
  without a TE line); programmatic navigation cuts instead.
- The German **voice clips are not distributed**; without them the device plays its chime.
- `TODO(verify)` hardware items remain (brightness command, IMU/battery stubs) — none affects the
  protocol.
- **Not verified on the board at the time of release `0.2.0`** (tracked in `testing.md`, "Board verification"):
  the OTA transfer end to end, UiConfig write + persistence, audible clean playback, factory reset —
  and the new idle / offline screens have only run in the simulator.

## Decisions taken (maintainer, 2026-10-24)

1. **UUID base frozen** as in `ble-protocol.md` ([`contract/uuid-proposal.md`](contract/uuid-proposal.md)).
2. **Security level stays open for v1** — no encryption/passkey/signing for now
   ([`update-authenticity.md`](update-authenticity.md) keeps the options for later).
3. **No report lock** — the app cannot lock the Melden page dynamically; the only switch is the
   persistent UiConfig bit ([`contract/melden-lock-proposal.md`](contract/melden-lock-proposal.md), withdrawn).
4. **Pairing QR** points at the repository page (`BSP_PAIR_URL`); the advertised name stays `BlitzerdeDsp`.
5. **No voice clips are shipped**; the device plays its chime without them.
