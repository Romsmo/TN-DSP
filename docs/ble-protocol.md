# BLE protocol

The board is a **BLE peripheral**. The companion app is the central. All data is **CBOR**: each
payload is a map with **integer keys**; **key 0 is the protocol version** (`1`). Unknown keys are
**ignored, never rejected** — that is how the protocol stays forward-compatible.

Reference implementations: `core/codec/protocol.cpp` (device), `tools/fake_app.py` (Python app
stand-in), `protocol/dart/` (tested pure-Dart package).

> **UUIDs are PROVISIONAL** — base `079aXXXX-d673-469c-98a0-12c0c5eb43b3`; a proposal for the
> final base is in [`contract/uuid-proposal.md`](contract/uuid-proposal.md). One place on the device:
> `firmware/components/transport_ble/ble_uuids.h` (and `uuidBase*` in the Dart package).

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

## Contract status

| | |
|---|---|
| Protocol version | **1** (key 0 of every CBOR map) |
| Contract revision | **1.0.0-draft** — becomes `protocol-v1.0.0` once the open items below are decided |
| Frozen | characteristic slots, every CBOR key number and type, flag bits, event tags, `HazardType` values, capability bits, OTA/Asset opcodes, `UiConfig` bit numbers — verified by the golden vectors in [`docs/contract/vectors/`](contract/vectors/) |
| **Not frozen yet** | (1) the UUID base — see [`contract/uuid-proposal.md`](contract/uuid-proposal.md); (2) the security level — see *Security level* below; (3) an optional additive "report lock" flag — see [`contract/melden-lock-proposal.md`](contract/melden-lock-proposal.md) |

Machine-readable companions (generated from the firmware / reference code, checked in CI):
`docs/contract/vectors/*.json` (golden vectors, categories, constants),
`docs/contract/sign-rules.json` (sign placement and replacement), and the tested Dart package in
[`protocol/dart/`](../protocol/dart/). Who does what: [`contract/responsibilities.md`](contract/responsibilities.md).

## Security level (as built — read this before relying on OTA)

- **No characteristic requires an encrypted, bonded or authenticated link.** The device accepts one
  connection at a time from *any* BLE central in range, and that central can read and write every
  characteristic — including **OTA (`0007`)** and **Asset (`0008`)** — without pairing.
- LE Secure Connections and bond persistence are compiled in. Bonds are stored in NVS (up to 3)
  *if the central starts pairing* ("Just Works": encryption without passkey, so no protection
  against an active man-in-the-middle). The firmware itself neither requests pairing nor restricts
  advertising to bonded centrals, and shows no passkey.
- Therefore the SHA-256 in the OTA `begin` frame protects against **transfer errors only**: whoever
  can connect can push an image together with its own matching hash. Authenticity options and a
  recommendation: [`update-authenticity.md`](update-authenticity.md).
- Proposed, **not implemented** (maintainer decision): require an encrypted (bonded) link for all
  writes; use a passkey shown on the AMOLED so pairing is authenticated; verify a signature on OTA
  images. Treat the device as *trusted environment only* until then.

## Delivery semantics

| Topic | Rule |
|---|---|
| State write type | write **without** response: the app gets no error if the device rejects a packet |
| Latest wins | the device keeps **one** pending State; if a newer packet arrives before the display loop polled (≈ every 20–60 ms), the older one is replaced and never rendered |
| Send rate | on every relevant change; steady state **≤ 2 packets/s**, bursts up to ~10/s. There is no throttle in the firmware — excess packets are simply superseded |
| Keep-alive | send a State at least every **5 s** even if nothing changed (a new `stateSeq`). After **8 s** without a State the device shows the "no app" badge and renders from its cache (build option `CONFIG_BDSP_STATE_WATCHDOG_S`, default 8) |
| Edge-triggered fields | `audioCue` and `confirmPromptActive` can be lost with a superseded packet. Keep the **same** `audioCue` (same `seq`) in the following State packets for ≥ 1 s — the device plays only when `seq` changes. Keep `confirmPromptActive` set for the whole timeout; the device counts down by itself |
| Heartbeat | the device notifies `Heartbeat{uptimeS}` every 10 s on `0002` while the app is subscribed |
| Settings / UiConfig / Time | write **with** response; a payload that does not decode is answered with ATT error `0x04` (Invalid PDU) |

### `stateSeq`

- `uint32`, **per connection**. The device drops a State with `0 < seq ≤ last accepted` **silently**.
- Start at **1** on every new connection (the device clears its high-water mark on connect *and*
  on disconnect, so an app restart or a reconnect never freezes the display). Increase by at least 1
  per packet; never reuse a value. Wrap-around is not expected within a session (4.29 · 10⁹ packets);
  if it ever happens, reconnect.
- `seq = 0` means "unsequenced": always accepted, and it resets the mark. Do not use it in production.
- A repeated State with unchanged content and a higher `seq` is the keep-alive.

### Sizes and MTU

- CBOR payloads are ≤ **244 bytes** (`maxPayloadBytes`; the device buffers 244 B, longer writes are
  truncated and fail to decode). A full State with 4+4 signs, a hazard and a cue is ≈ 80 B.
- The device asks for ATT MTU **256**. The app must use the **negotiated** MTU: a write holds at most
  `MTU − 3` bytes, and iOS may cap write-without-response lower than the device allows.
- OTA / Asset frames: **≤ 244 bytes including the opcode**. Use
  `chunk = min(negotiated MTU − 3, 244) − 1` data bytes per frame (the PC tool uses 230).
- Pacing (tested with `tools/fake_app.py`): after an OTA `begin`, **wait** for the device's first
  progress notification (0 bytes — it has erased the target slot, which takes seconds), then pause
  ~40 ms after every 10 frames. Assets: same pacing, wait for the `ok` notification after `end`,
  250 ms between files (the SD card shares a bus with the display).

### Defaults for missing fields (decoder behaviour, pinned by the vectors)

| Message | Missing key → |
|---|---|
| State | flags 0 · no speed sign · `speedLimitUnit` km/h · `confirmPromptTimeoutS` **15** · `stateSeq` 0 · no signs · no cue |
| AudioCue | `seq` 0 (never plays) · mode tones · event info · unit km/h |
| Settings | sound off · tones · volume **70** · theme 0 · km/h · `filterBits` **0xFFFF** · night 20–06 · cache 10 min |
| UiConfig | all features on |
| Time | epoch 0 · offset 0 |

Unknown keys of any supported CBOR type are skipped; a payload that is not a CBOR map is rejected.

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
| 11 | array&lt;uint&gt; | leftSigns | VzNr codes (`major*100+minor`), prohibitions by convention, ≤ 4 on the wire; the display shows the **newest 3** (last entries). Which sign goes on which side and which signs replace each other is the **app's** job: [`contract/sign-rules.json`](contract/sign-rules.json) |
| 12 | array&lt;uint&gt; | rightSigns | VzNr codes, danger signs by convention, ≤ 4 (display: newest 3). An unknown code renders an empty slot |
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
| 1 | uint | tag: 0 ReportPressed · 1 ReportSubmitted · 2 ConfirmResponse · 3 SettingsPhoneConnect (legacy: no longer sent by the UI) · 4 SettingsSoundToggle · 5 SettingsThemeChanged · 6 SettingsUnitChanged · 7 SettingsFilterChanged · 8 Heartbeat |
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
bit3 hasSD · bit4 speedcamBuild · bit5 hasOta) · `3` width · `4` height. `hasSD` is set only when a
card is mounted at boot; `speedcamBuild` is clear in firmware built without any speed-camera code
(then hazard ids 0–4 do not exist on that device).

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

Bonding: see *Security level* above — the firmware does not require or request it today. If a
central bonds by itself, the bond is persisted in NVS (`CONFIG_BT_NIMBLE_NVS_PERSIST`, up to 3).

## Connect sequence and failure behaviour

Order the app follows after every (re)connect:

1. **Scan** for the service UUID (it is in the advertising packet; the device *name* is only in the scan
   response and is not an identity). One connection at a time; a second central cannot connect.
2. **Connect**, let the OS negotiate the MTU (device asks for 256), discover the service.
3. **Read DeviceInfo `0003`**: check `protoVersion` (key 0) equals the app's supported major version,
   read `fwVersion` and the `caps` bits (what the app does when a bit is missing is in
   [`contract/responsibilities.md`](contract/responsibilities.md)).
4. **Subscribe** to notifications of Event `0002`, Settings `0004` and UiConfig `0006`
   (OTA `0007` / Asset `0008` only while a transfer runs).
5. **Write Time `0005`** (epoch seconds + UTC offset in minutes).
6. **Sync Settings `0004`**: read it, then write the app's authoritative set. A Settings write
   replaces **all** fields — a key you leave out reverts to its default (table above), so always send
   the complete map.
7. **Read UiConfig `0006`**; write it only if the app wants to change a feature switch.
8. **Stream State `0001`**, `stateSeq` starting at **1**.

| Situation | Device | App |
|---|---|---|
| App goes away / link drops | shows the "no app" badge after the watchdog, renders the last state from its cache, re-advertises, accepts the next central | reconnect and run the sequence again; **restart `stateSeq` at 1** |
| App is silent for > 8 s | same badge + cache, link stays up | send keep-alive States (≥ every 5 s) |
| Link drops during OTA | aborts, discards the partial image, keeps running the old firmware | restart from `begin` |
| Link drops during an Asset upload | closes the partial file — it **stays on the card, incomplete** | re-send the file (the next `begin` overwrites it) |
| OTA accepted (`0x11`) | reboots ~0.6 s later; the link drops by design; the new image must run ~3 s to confirm itself, otherwise the next reset **rolls back** | reconnect after ~5–10 s, read DeviceInfo; if `fwVersion` did not change, report "update failed" |
| "Zurücksetzen" pressed on the device | erases settings, UiConfig, cache **and bonds**, reboots to the pairing screen | re-run the sequence; the phone's own bond record is not removed by the device (iOS: the user must *Forget This Device* in Settings → Bluetooth; an app cannot do it) |

Pairing and bonding: see *Security level* above. Today nothing requires encryption, so a stale bond
after a factory reset is harmless; if an encrypted link is made mandatory later, a stale bond becomes
visible and the app must guide the user through forgetting the device. iOS pairs automatically when a
protected characteristic is accessed (an app cannot start pairing itself); on Android the app can call
`createBond`.

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
