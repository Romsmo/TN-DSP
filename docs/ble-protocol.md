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
| `0006` | `079a0006-…` | **UiConfig** | Read / Write / Notify | both |
| `0007` | `079a0007-…` | **OTA** | Write / Notify | both |
| `0008` | `079a0008-…` | **Asset** | Write / Notify | both |

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
`8` cacheStaleMinutes · `9` soundVolume (uint, 0–100; clamped; applied live to the codec).

Key `9` was added after v1; a writer that omits it leaves the device at its stored volume
(default 70). The NVS blob is format v2 — a v1 blob still loads, volume falls back to 70.

The per-category warning filter lives in the app; the device also persists these bits to NVS and
suppresses overlays of disabled categories as a second layer.

## UiConfig  (`0006`, read / write / notify — dev feature switches)

`0` version · `1` **bits** (uint32; **bit set = feature ON**, so a fresh device with no stored
config reads all-on and behaves normally).

| Bit | Feature | Off means |
|---|---|---|
| 0 | `meldenPage` | no "Melden" tile, the "+" is hidden, no left-swipe |
| 1 | `settingsPage` | the gear tap does nothing |
| 2 | `hazardOverlay` | hazard packets update the sign but raise no overlay |
| 3 | `confirmOverlay` | the "Gesehen?" prompt never appears |
| 4 | `signColumns` | the left/right StVO sign columns are hidden |
| 5 | `directionArrows` | no "free road" picture (the wire flag is still `directionArrows`) |
| 6 | `peerCount` | no "480K" under the globe |
| 7 | `gearButton` | the Settings gear is hidden (independent of bit 1) |
| 8 | `connectQr` | "Handy verbinden" shows no QR |
| 9 | `bootAnimation` | no road sweep at power-on |

The device persists the config to NVS and echoes it on notify, so a reconnecting app (or a second
client) reads back the current set. Changing it takes effect on the next render.

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

## OTA  (`0007`, write-without-response / notify — firmware update)

Raw framing (**not CBOR** — the payload is a firmware image, so every byte counts). First byte is
the opcode.

**App → device**

| Op | Bytes | Meaning |
|---|---|---|
| `0x01` begin | `[1..4]` total length (uint32 LE) · `[5..36]` SHA-256 of the image | start; picks the idle OTA slot |
| `0x02` data | `[1..]` raw image bytes, **sent in order** | appended; device buffers 4 KB then writes to flash |
| `0x03` end | — | flush, verify the SHA, set the boot partition, reboot after 0.6 s |
| `0x04` abort | — | discard |

**Device → app** (notify)

| Op | Bytes | Meaning |
|---|---|---|
| `0x10` progress | `[1..4]` bytes received (uint32 LE) | every ~32 KB |
| `0x11` ok | — | image accepted, rebooting |
| `0x12` error | `[1]` code (1 no slot · 2 begin · 3 write · 4 SHA · 5 end) | aborted |

Partition table is dual-slot (`ota_0` / `ota_1` + `otadata`, `firmware/partitions.csv`). The new
image boots once in *pending-verify*; `app_main` calls `esp_ota_mark_app_valid_cancel_rollback()`
after ~3 s of running, so a broken update rolls back on the next reset
(`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`). A disconnect mid-transfer aborts cleanly.
`tools/fake_app.py` pushes an image with `ota <path/to/blitzerdsp.bin>`. The `HAS_OTA` capability
bit in DeviceInfo is set whenever the running firmware has the dual-slot layout.

## Asset  (`0008`, write-without-response / notify — file → SD card)

Same raw framing as OTA, for copying audio clips (and later images) onto the card.

| Op | Bytes | Meaning |
|---|---|---|
| `0x01` begin | `[1]` path length · `[2..]` path, relative to `/sdcard` (parent dirs are created) |
| `0x02` data | `[1..]` file bytes, in order |
| `0x03` end | close the file |
| `0x04` abort | discard |

Device notifies `0x11` ok / `0x12` error. `tools/fake_app.py`: `upload <dir> [prefix]` or
`upload <file> <remote>`; `tools/gen_voice.py --upload` regenerates the voice clips and pushes the
whole `/audio/` tree.
