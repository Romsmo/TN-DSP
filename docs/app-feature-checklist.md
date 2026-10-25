# App feature checklist — everything the display offers

Hand this to the app side and tick it off. It lists **every function of the display (firmware
0.2, contract 1.1.0)**, what the app has to send or handle for it, where it is specified, and how to
test it. If a function is not on this list, the display does not have it (section 13 lists what is
**missing** compared with a full traffic app).

**Legend** — **R** required for a working driving app · **O** optional / nice to have · **D** developer
or test use only. Wire references use the characteristic slot (`0001`…`0008`) and CBOR key number from
[`ble-protocol.md`](ble-protocol.md); `vec:` names a golden vector in
[`contract/vectors/`](contract/vectors/) that pins the bytes. The Dart package
[`protocol/dart/`](../protocol/dart/) already implements everything marked "package".

---

## 1. Connection and session

| ☐ | ID | Function | The app must | Wire / reference | Level |
|---|---|---|---|---|---|
| ☐ | A1 | Find the display | scan for the service UUID `079a0000-d673-469c-98a0-12c0c5eb43b3` (advertised; the name `BlitzerdeDsp` is only in the scan response) | ble-protocol *Connect sequence* | R |
| ☐ | A2 | One client at a time | handle "connect refused / busy" (a second central cannot connect) | — | R |
| ☐ | A3 | MTU | use the **negotiated** MTU; write at most `MTU − 3` bytes; the device asks for 256 and accepts ≤ 244-byte frames | *Sizes and MTU* | R |
| ☐ | A4 | Read **DeviceInfo** first | read `0003`; check `protoVersion == 1`, store `fwVersion`, `caps`, display size 466 × 466 | `0003`, package `DeviceInfo` | R |
| ☐ | A5 | Version gate | newer protocol major → do **not** stream, ask the user to update the app; older than supported → ask to update the display | [`compatibility.md`](compatibility.md) | R |
| ☐ | A6 | Capability bits | respect `caps`: audio, rtc, sd, speedcamBuild, ota (see §11) | `0003` key 2 | R |
| ☐ | A7 | Subscribe | notifications of Event `0002`, Settings `0004`, UiConfig `0006` | *Connect sequence* | R |
| ☐ | A8 | Write **Time** after every connect | epoch seconds + UTC offset minutes | `0005`, vec: `time.json` | R |
| ☐ | A9 | Sync **Settings** after every connect | read, then write the **complete** set (a missing key reverts to its default) | `0004` | R |
| ☐ | A10 | Reconnect | after any drop, run A4–A9 again and **restart `stateSeq` at 1** | *Connect sequence* | R |
| ☐ | A11 | Security level | know that the link is **open** (no pairing/encryption needed); do not rely on bonding | *Security level* | R |

## 2. Trip mode and presence (the display's three states)

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | B1 | **Trip off** (connected, no trip) | send `driveActive = false`: the display shows the **TNV logo** and only Settings; hazards/prompts/announcements are ignored | State key 14 · vec: `trip-off-idle-logo` | R |
| ☐ | B2 | **Trip on** | send `driveActive` true or omit it: the full driving UI | State key 14 · vec: `drive-key-absent-means-active` | R |
| ☐ | B3 | Keep-alive while idle | keep sending State (≥ every 5 s) also with the trip off — after 8 s of silence the display drops to **Offline** | *Delivery semantics* | R |
| ☐ | B4 | **Offline** is the display's own state | expect: no link / silence → black screen, grey crossed-out globe, only Settings; Melden unavailable | ui-spec 5.2 | R |
| ☐ | B5 | Melden only on a trip | the display refuses Melden and closes an open one when the trip ends or the link drops | — | — |

## 3. The driving screen — what each State field renders

All fields are in the State characteristic `0001` (write without response). Encode with the package's
`DisplayState.encode()`.

| ☐ | ID | State field (key) | On the display | Notes | Level |
|---|---|---|---|---|---|
| ☐ | C1 | `speedLimit` (2) + `speedLimitUnit` (3: 0 km/h, 1 mph) | the big speed-limit sign, white disc with red ring; absent → no sign | a **sign value**, shown unchanged (a UK 30 mph sign stays 30); up to 3 digits | R |
| ☐ | C2 | `internetConnected` (flags1 bit 0) | globe icon; when false the globe is **crossed out in red** and the peer number disappears | | R |
| ☐ | C3 | `networkPeers` (10) | abbreviated under the globe: 480000 → "480K", 1.5 M → "1.5M" | | O |
| ☐ | C4 | `directionArrows` (flags1 bit 1) | the **free-road picture** above the sign (hidden while a hazard overlay is up) | "freie Fahrt" | R |
| ☐ | C5 | `leftSigns` (11) / `rightSigns` (12) | StVO sign columns beside the speed sign; ≤ 4 on the wire, the display shows the **newest 3** per column; unknown code → empty slot | the app picks columns and replacements — see §4 | R |
| ☐ | C6 | `nightMode` (flags1 bit 4) | dims the panel (brightness 0xFF ↔ 0x50); the theme stays dark | the app decides when; `nightHours` in Settings are stored but **not used** by the display | O |
| ☐ | C7 | `useMph` (flags1 bit 5) | display preference: the units button in Settings follows it | **not** the sign unit | O |
| ☐ | C8 | `epochS` (9) | cache ageing (no visible effect today) | send it anyway | O |
| ☐ | C9 | `stateSeq` (8) | stale/duplicate packets are dropped | strictly increasing from 1 per connection | R |

Sending rhythm: on every change, ≥ every 5 s as keep-alive, steady ≤ 2/s (the device keeps only the
latest packet).

## 4. Context signs

| ☐ | ID | Function | The app must | Reference | Level |
|---|---|---|---|---|---|
| ☐ | D1 | Side | prohibition/regulation signs (VzNr 2xx) go **left**, danger signs (1xx) **right** | [`contract/sign-rules.json`](contract/sign-rules.json) · package `signGoesLeft` | R |
| ☐ | D2 | Replacement families | adding a sign evicts conflicting ones in **both** columns (Halteverbot 283/286, Überholverbot 276/277/280/281, curves, slopes, all 101-x, same major VzNr, 282 ends 274/276/277) | package `addSign`, vec: `vectors/sign-rules.json` (9 scenarios + 58 × 58 table) | R |
| ☐ | D3 | Newest wins | keep ≤ 3 per column; adding a 4th drops the oldest | package `addSign` | R |
| ☐ | D4 | Wire code | `major × 100 + minor` (`142-10` → 14210) | package `signCodeOf` | R |
| ☐ | D5 | Available signs | only the **58 vendored signs** render; list with official German names in `sign-rules.json` | `signs[]` | R |

## 5. Hazards and speed cameras

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | E1 | Hazard overlay | set `hazardActive` (flags1 bit 2) and `hazardType` (4): blinking category icon, red accents, dimmed page | vec: `hazard-mobile-camera-with-voice-cue` | R |
| ☐ | E2 | Distance bar | send `hazardDistanceM` (5): vertical bar + "600m" label; the bar's range is the largest distance seen for this hazard | | R |
| ☐ | E3 | Enforced limit | send `hazardSpeedLimit` (6): the speed sign switches to it while the overlay is up | | O |
| ☐ | E4 | **11 categories** (ids 0–10) | use the frozen ids; the display has an icon and German label for each | [`vectors/categories.json`](contract/vectors/categories.json) | R |
| ☐ | E5 | Unknown type | a `hazardType` > 10 renders a generic "!" icon, never an error | vec: `hazard-unknown-type-11` | O |
| ☐ | E6 | Speed-camera group (ids 0–4) | only when `caps` has `speedcamBuild`; otherwise never send them and hide them in the filter UI | `caps` bit 4 | R |
| ☐ | E7 | User filter | write `filterBits` (Settings key 5, bit n = category n warned about); the display also suppresses disabled categories as a second layer; unknown types are always shown | vec: `settings.json` | R |
| ☐ | E8 | Overlay can be stripped | UiConfig bit 2 `hazardOverlay` off → no overlay is raised | §9 | D |

Category list (id · key · German label · group):
0 `FixedSpeedCamera` Fest · 1 `MobileSpeedCamera` Mobil · 2 `TrailerCamera` Anhänger · 3 `RedLightCamera`
Rotlicht · 4 `DistanceControl` Abstand · 5 `Traffic` Stau · 6 `Ice` Glätte · 7 `Accident` Unfall ·
8 `Construction` Baustelle · 9 `Breakdown` Panne · 10 `Obstacle` Hindernis. (0–4 = speed-camera group.)

## 6. "Gesehen?" prompt

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | F1 | Raise the prompt | set `confirmPromptActive` (flags1 bit 3) and `confirmPromptTimeoutS` (7, default 15); the display shows Ja / Nein with a countdown ring and **locks navigation** | vec: `confirm-prompt-night-mph` | R |
| ☐ | F2 | Keep it set | the packet can be superseded: keep the flag set for the whole timeout — the device counts down itself | *Delivery semantics* | R |
| ☐ | F3 | Receive the answer | Event `ConfirmResponse{seen}` (key 4) on `0002`; a timeout is **silent** (no event) | vec: `confirm-seen`, `confirm-not-seen` | R |
| ☐ | F4 | Prompt can be stripped | UiConfig bit 3 `confirmOverlay` | §9 | D |

## 7. Reporting (Melden)

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | G1 | Know when the page is opened | Event `ReportPressed` (tag 0) — sent when the user opens Melden (swipe or "+") on a trip | vec: `report-pressed` | O |
| ☐ | G2 | Receive a report | Event `ReportSubmitted` (tag 1): `hazardType` (key 2) and, for categories that need a speed (1 Mobil, 2 Anhänger, 4 Abstand), `speedKmh` (key 3) | vec: `report-submitted-with-speed`, `…-no-speed` | R |
| ☐ | G3 | Speed value | the display sends the **last speed-limit number it received** for speed categories. *Known issue:* when the limit's unit was mph that number is mph, not km/h — a fix (convert, round to 5) is planned | — | R |
| ☐ | G4 | Page can be stripped | UiConfig bit 0 `meldenPage` (persistent) removes the page, the "+" and the swipe | §9 | D |
| ☐ | G5 | No driving lock | the display does **not** lock Melden by speed; only the trip flag (§2) matters. A lock was considered and dropped | [`contract/melden-lock-proposal.md`](contract/melden-lock-proposal.md) | — |

## 8. Audio

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | H1 | Announcements | send `audioCue` (13): `seq` (new value = play), `mode` (0 tones, 1 voice), `event`, optional `hazardType`, `distanceM`, `speedValue`, `speedUnit` | vec: `audio-cue-tone` | R |
| ☐ | H2 | Events | 0 HazardApproach · 1 HazardImminent · 2 ConfirmPrompt · 3 Cleared · 4 Info | [`categories.json`](contract/vectors/categories.json) `audio.events` | R |
| ☐ | H3 | Repeat the cue | keep the same `audioCue` (same `seq`) in following States for ≥ 1 s so a superseded packet cannot lose it | *Delivery semantics* | R |
| ☐ | H4 | Voice composition | "Achtung" + category phrase + (if `distanceM` ≥ 50) "in" + distance bucket **rounded down** (100/200/300/500/800 m) | `categories.json` | O |
| ☐ | H5 | Voice clips optional | the clips are **not shipped**; without them the display plays the event chime (also without an SD card) | `caps` bit 3, [`audio-assets.md`](audio-assets.md) | R |
| ☐ | H6 | Master gate | `soundEnabled` (Settings key 1) must be true, else nothing plays; no cue is played outside a trip | | R |
| ☐ | H7 | Volume | `soundVolume` (Settings key 9, 0–100, live) | vec: `custom`, `volume-100` | R |
| ☐ | H8 | Upload voice clips | push WAV files (16 kHz mono 16-bit) over Asset `0008` into `audio/voice/de/…` | §11 | O |

## 9. Settings and feature switches

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | I1 | Settings fields | `soundEnabled` 1 · `soundMode` 2 · `theme` 3 (ignored: one dark theme) · `useMph` 4 · `filterBits` 5 · `nightStartHour` 6 / `nightEndHour` 7 (stored, **unused**) · `cacheStaleMinutes` 8 (no visible effect) · `soundVolume` 9 | `0004` · package `DeviceSettings` | R |
| ☐ | I2 | On-device toggles | the user can switch **Ton** and **Einheiten** on the display itself: the device emits `SettingsSoundToggle` / `SettingsUnitChanged` **and** a Settings notify — the app must reflect them | `0002` tags 4, 6; `0004` notify | R |
| ☐ | I3 | Factory reset on the device | Settings → *Zurücksetzen* wipes settings, UiConfig, cache and bonds and reboots to the pairing screen; the app sees a disconnect and must run the connect sequence again; the phone's own bond record is **not** removed | ble-protocol *Connect sequence* | R |
| ☐ | I4 | UiConfig (developer switches) | read/write `0006` bits: 0 meldenPage · 1 settingsPage · 2 hazardOverlay · 3 confirmOverlay · 4 signColumns · 5 directionArrows · 6 peerCount · 7 gearButton · 8 connectQr · 9 bootAnimation (set = on; persistent) | vec: `ui-config.json` · package `UiConfig` | D |

## 10. Time and wall clock

| ☐ | ID | Function | The app must | Wire | Level |
|---|---|---|---|---|---|
| ☐ | J1 | Set the clock | write `Time` after connect (epoch + UTC offset in minutes, may be negative); stored in the battery-backed RTC (`caps` bit 2) | vec: `utc-plus-2`, `utc-minus-3-30` | R |

## 11. Firmware updates and files

| ☐ | ID | Function | The app must | Wire / reference | Level |
|---|---|---|---|---|---|
| ☐ | K1 | Detect an update | fetch the release `manifest.json`; compare `firmware.version` with `DeviceInfo.fwVersion`; check `minFromVersion` and `protocol.minAppProtocol` | [`release-format.md`](release-format.md) | O |
| ☐ | K2 | Only if `hasOta` | offer an update only when `caps` bit 5 is set | `0003` | O |
| ☐ | K3 | Push the image | OTA frames: `begin` (length + SHA-256) → **wait for the "0 bytes" progress notify (slot erase)** → paced data frames → `end`; handle progress `0x10`, ok `0x11`, error `0x12` + code | `0007` · package `OtaFrames` / `OtaNotify` | O |
| ☐ | K4 | After the reboot | reconnect after ~5–10 s, read DeviceInfo, confirm the new `fwVersion`; unchanged → "update failed" (the device rolled back) | | O |
| ☐ | K5 | Image trust | the device only checks the SHA-256 the app supplies; verify the manifest/image yourself (signature planned) | [`update-authenticity.md`](update-authenticity.md) | O |
| ☐ | K6 | File upload | Asset frames: `begin` path → data → `end`; notify ok/error; a dropped link leaves an incomplete file | `0008` · package `AssetFrames` | O |
| ☐ | K7 | The display shows progress itself | a full-screen "Update NN %" layer while an image streams in | — | — |

## 12. What the display does on its own (the app only needs to know)

- **Pairing screen** on first boot / after a reset: "Verbinde dein Mobilgerät", a QR code to the
  repository page, the three steps *Bluetooth einschalten · mit "BlitzerdeDsp" verbinden · App öffnen*.
- **Boot animation** (the road picture sweeping up), skippable via UiConfig bit 9.
- **Settings page** (always reachable by the gear): Ton, Einheiten, Zurücksetzen, Updates (an info
  panel), version/protocol/link footer.
- **Heartbeat** Event (tag 8, `uptimeS`) every 10 s while subscribed.
- **Offline cache**: stores the last State in NVS, but shows nothing from it without an app.
- **All texts are German**; one dark theme; the BOOT button long-press toggles a built-in demo.

## 13. Not supported by the display (compare with the Trafficnetwork feature list)

If the Trafficnetwork app has any of these, the protocol would need an additive extension:

- the **vehicle's own speed** — only the speed *limit* is shown;
- **more than one hazard** at a time, a hazard list or "next hazards";
- hazard details: direction, lane, age, confidence, average-speed ("Streckenradar") sections as their own type;
- **navigation**: route, turn instructions, lane guidance, ETA, maps;
- free-text messages, notifications, images beyond the 58 StVO signs and the 11 category glyphs;
- other languages, themes, or per-user content;
- battery/charging or motion data (IMU and battery are stubs).

## 14. How to verify the app against this list

1. Run the package tests: `cd protocol/dart && dart test` (88 tests against every golden vector).
2. Compare your encoder output byte-for-byte with the `canonical` vectors for State, Settings, Time
   and UiConfig.
3. Use the PC console as a reference peer: `python tools/fake_app.py` (GUI, every field above) or
   `--repl` (`drive`, `online`, `hazard`, `confirm`, `cue`, `set`, `ui`, `ota`, `upload`, …).
4. Walk the on-device checklist in [`testing.md`](testing.md) with the real app instead of the console.
