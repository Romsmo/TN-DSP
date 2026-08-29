# BLE protocol

The board is a **BLE peripheral**. The companion app is the central. All data is **CBOR**: each
payload is a map with **integer keys**; **key 0 is the protocol version** (`1`). Unknown keys are
**ignored, never rejected** — that is how the protocol stays forward-compatible.

Reference implementations: `core/codec/protocol.cpp` (device), `tools/fake_app.py` (Python app
stand-in), `docs/app-reference/blitzerdsp_protocol.dart` (Flutter sketch).

> **UUIDs are PROVISIONAL** — base `079aXXXX-d673-469c-98a0-12c0c5eb43b3`, finalized at M8.
> One place: `firmware/components/transport_ble/ble_uuids.h`.

## Service & characteristics

| Slot | UUID | Name | Properties | Direction |
|---|---|---|---|---|
| `0000` | `079a0000-…` | Service | — | — |
| `0001` | `079a0001-…` | **State** | Write Without Response | app → device |
| `0002` | `079a0002-…` | **Event** | Notify | device → app |
| `0003` | `079a0003-…` | **DeviceInfo** | Read | device → app |
| `0004` | `079a0004-…` | **Settings** | Read / Write / Notify | both |
| `0005` | `079a0005-…` | **Time** | Write | app → device |

Payloads fit one ATT packet (`kMaxPayload = 244`, targeting a 247-byte MTU). A typical State
packet is 25–50 bytes.

## State  (`0001`, app → device)

| Key | Type | Field | Notes |
|---|---|---|---|
| 0 | uint | protocol version | always `1` |
| 1 | uint | flags1 | bit0 internetConnected · bit1 directionArrows · bit2 hazardActive · bit3 confirmPromptActive · bit4 nightMode · bit5 useMph |
| 2 | uint | speedLimit | omit → no sign. A **sign value**, shown as-is |
| 3 | uint | speedLimitUnit | 0 km/h · 1 mph |
| 4 | uint | hazardType | 0–10 (frozen, see below); only when hazardActive |
| 5 | uint | hazardDistanceM | |
| 6 | uint | hazardSpeedLimit | e.g. mobile-camera limit |
| 7 | uint | confirmPromptTimeoutS | default 15 |
| 8 | uint | stateSeq | monotonic; the device drops `seq ≤ last` |
| 9 | uint | epochS | app wall-clock, for cache aging |
| 10 | uint | networkPeers | raw count (the "480K" abbreviation is the device's job) |
| 11 | array&lt;uint&gt; | leftSigns | VzNr codes (`major*100+minor`), prohibitions by convention, ≤ 4 |
| 12 | array&lt;uint&gt; | rightSigns | VzNr codes, danger signs by convention, ≤ 4 |
| 13 | map | audioCue | see below; the device plays it on a new `seq` |

**audioCue sub-map:** `0` seq · `1` mode (0 tones / 1 voice) · `2` event (0 approach · 1 imminent ·
2 confirm · 3 cleared · 4 info) · `3` hazardType? · `4` distanceM? · `5` speedValue? · `6` speedUnit.

**HazardType (frozen for v1):** 0 fixedSpeedCamera · 1 mobileSpeedCamera · 2 trailerCamera ·
3 redLightCamera · 4 distanceControl · 5 traffic · 6 ice · 7 accident · 8 construction ·
9 breakdown · 10 obstacle. A value > 10 renders a generic icon.

### Example

`{0:1, 1:7, 2:60, 3:0, 4:1, 5:300, 6:50, 7:15, 8:7, 10:480000, 11:[27600], 12:[14210]}`
→ `ac 0001 0107 02183c 0300 0401 0519012c 061832 070f 0807 0a1a00075300 0b81196bd0 0c81193782`
(38 bytes: connected + arrows + hazard, 60 sign, mobile camera 300 m / 50, overtaking-ban left,
Wildwechsel right.)

## Event  (`0002`, device → app, Notify)

| Key | Type | Field |
|---|---|---|
| 0 | uint | protocol version |
| 1 | uint | tag: 0 ReportPressed · 1 ReportSubmitted · 2 ConfirmResponse · 3 SettingsPhoneConnect · 4 SettingsSoundToggle · 5 SettingsThemeChanged · 6 SettingsUnitChanged · 7 SettingsFilterChanged · 8 Heartbeat |
| 2 | uint | hazardType (ReportSubmitted) |
| 3 | uint | speedKmh (ReportSubmitted, optional — km/h, rounded to 5) |
| 4 | bool | seen (ConfirmResponse) |
| 5 | bool | on (SettingsSoundToggle) |
| 6 | uint | mode (SettingsThemeChanged) |
| 7 | bool | useMph (SettingsUnitChanged) |
| 8 | uint | filterBits (SettingsFilterChanged) |
| 9 | uint | uptimeS (Heartbeat, every 10 s) |

## Settings  (`0004`, read / write / notify — bidirectional)

`0` version · `1` soundEnabled (bool) · `2` soundMode (0 tones / 1 voice) · `3` theme (0 auto /
1 day / 2 night — currently ignored, single dark theme) · `4` useMph (bool) · `5` filterBits
(uint16, bit *n* = HazardType *n* enabled) · `6` nightStartHour · `7` nightEndHour ·
`8` cacheStaleMinutes.

The per-category warning filter lives in the app; the device also persists these bits to NVS and
suppresses overlays of disabled categories as a second layer.

## DeviceInfo  (`0003`, read)

`0` version · `1` fwVersion (text) · `2` caps (bitmask: bit0 hasAudio · bit1 hasImu · bit2 hasRtc ·
bit3 hasSD · bit4 speedcamCompiled) · `3` width · `4` height.

## Time  (`0005`, write)

`0` version · `1` epochS · `2` tzOffsetMin (signed). Written once after connect; sets the PCF85063.

## Connection lifecycle

```
        ┌─────────────┐  no bond / disconnected   ┌──────────────┐
        │  ADVERTISING │ ────────────────────────► │ (device idle) │
        └──────┬──────┘                            └──────────────┘
   central connects │
                    ▼
        ┌─────────────┐  app reads DeviceInfo, writes Time,
        │  CONNECTED  │  subscribes Event, syncs Settings, streams State
        └──────┬──────┘
   no State for  │  N s (watchdog)                link drop
   ──────────────┼──────────────────────────────────────────────
                 ▼
        device shows "no app" badge, renders from OfflineCache;
        re-advertises; auto-reconnects to a bonded central.
```

Bonding is used (LE Secure Connections, "just works"). Bonds are persisted to NVS
(`CONFIG_BT_NIMBLE_NVS_PERSIST`) so a bonded central re-encrypts silently after a device reboot
(ignition cycle). The "Handy verbinden" screen is only shown before the first successful
connection or on explicit request. Pairing is the central's choice — an unbonded connection is
still accepted.

## Version compatibility

- Key 0 carries the **protocol version**. Additive changes (new keys, new enum values appended)
  **do not** bump it.
- A receiver **ignores keys it does not know** and **enum values out of its range** (hazard > 10 →
  generic icon; unknown event tag → dropped).
- The version is bumped only for a **breaking** change (a key's meaning or type changes, a field is
  removed). On a major mismatch the device keeps rendering the last good state and the app should
  surface an "update firmware / app" prompt (it can read the device's version from DeviceInfo).

## Firmware update (OTA) — `TODO`, not implemented

The device's Settings → "Updates" panel tells the user that firmware arrives over Bluetooth via
the app. The transport for that does not exist yet: no OTA characteristic, and the partition table
is still single-slot (`firmware/partitions.csv`). When built, add a chunked-upload characteristic
(or its own service), dual OTA slots + `otadata`, and an apply/verify/`esp_restart` path. Until
then, updates are a manual `idf.py flash` over USB. See the `blitzerdsp-ota-pending` note.
