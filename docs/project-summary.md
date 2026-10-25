# BlitzerdeDsp — project summary & current status

_Snapshot: 2026-10-25, firmware `0.2.0` released (tag `fw-v0.2.0`; earlier `fw-v0.1.0`), contract `protocol-v1.1.0` (repository `Romsmo/TN-DSP`). Host suite: 79 test cases / 584 assertions, Dart package 88 tests, all CI jobs green._

## 1. What it is

Open-source firmware for a dedicated in-car AMOLED display (BMW E90). It shows the current speed
limit, hazard / speed-camera warnings, context traffic signs and a touch UI for reporting hazards.
**All live data arrives over Bluetooth LE from a companion phone app — the board never talks to an
external API.** It paints the state it receives and emits raw interaction events back.

> **Guiding rule (binding): the display renders, it never computes.** No map-matching, no distance
> maths, no decision about *what* to show. The one grey area is audio: the app sends a structured
> cue (the *when*), the board composes the phrase / picks the tone (the *how it sounds*).

Project rules: plan-first · ask rather than guess · nothing invented (unverified facts are marked
`TODO(verify)`) · incremental and green (build + host tests + Conventional-Commits commit per
milestone) · code, comments, commits and docs in **English**, on-screen strings in **German** via
the i18n layer · strict layering · **one dark theme, always** (no day/night switching).

The companion **Flutter app is out of scope** for this repo. In its place:
[`tools/fake_app.py`](../tools/fake_app.py) — a PC test console (tkinter GUI + REPL) that speaks the
full BLE protocol — and a tested Dart protocol package in [`protocol/dart/`](../protocol/dart/) with the contract in [`docs/contract/`](contract/).

## 2. Hardware

**Waveshare ESP32-C6-Touch-AMOLED-1.43** (not the S3 variant the original brief described).

| Part | Detail |
|---|---|
| SoC | ESP32-C6, RISC-V **single core**, 160 MHz, **no PSRAM**, ~512 KB SRAM, 16 MB flash, BLE 5 only |
| Display | CO5300 QSPI AMOLED 466×466 round (driven through `esp_lcd_sh8601`), no TE line used |
| Touch | FT6x36 @ I²C 0x38, **polled** (INT/RST not wired) |
| Other I²C | PCF85063 RTC 0x51 · QMI8658 IMU 0x6B (stub) · TCA9554 IO-expander 0x20 (P7 = audio PA) · ES8311 codec 0x18 |
| Audio | ES8311 DAC + speaker over I²S0 (MCLK 19, BCLK 21, WS 22, DOUT 23). ES7210 mic ADC present, unused |
| SD card | SPI on **SPI2 shared with the LCD** (MOSI 4, MISO 5, CLK 11, CS 15), mounts at `/sdcard` |
| Buttons | BOOT (GPIO 9) — long-press toggles the mock demo |

Pin map lives in one place: [`firmware/main/board_config.h`](../firmware/main/board_config.h).
Details and sources: [`hardware.md`](hardware.md).

## 3. Software stack

ESP-IDF 5.3.2 · LVGL 9.2.2 (+ `esp_lvgl_port`) · C++17 · NimBLE · hand-rolled CBOR (integer keys,
versioned, unknown keys ignored) · `esp_codec_dev` for the ES8311 · Montserrat fonts · 58 StVO
sign SVGs (Wikimedia Commons, public domain) + 23 Lucide icons, rasterised to LVGL C arrays.

### Layers

```
firmware/main/app_main.cpp   composition root — wires everything, owns the only globals
  ├─ ui/        LVGL screens, widgets, overlays, i18n (de), theme — no IDF/SDL includes
  ├─ core/      model, AppStateMachine, CBOR codec, settings/cache/ui-config stores,
  │             audio service + cue composer, category registry — host-compilable, LVGL-free
  ├─ transport/ ITransport: BLE (firmware) · Mock (demo/sim) · Mux (routes between them)
  └─ hal/       interfaces: Display Touch Rtc Imu Nvs Audio Storage Buttons Clock Power
       impls:   firmware/components/hal_esp/ (target) · sim/ + test/fakes.h (host)
```

Data path on the device: `MuxTransport(BLE, Mock)` → `AppRunner` → `AppStateMachine` (→ `ViewModel`)
→ `ScreenManager` (a pure renderer). `sim/` is a second composition root that runs the same
`ui/` + `core/` on the desktop against LVGL-SDL — the fast iteration path.

### FreeRTOS tasks (single core)

| Task | Role |
|---|---|
| `app_main` loop (20 ms) | `AppRunner::step` → state machine → render under the LVGL lock |
| LVGL handler | `esp_lvgl_port` timer/handler task, recursive port mutex |
| NimBLE host | GATT peripheral |
| `audio` (prio 5, 6 KB) | streams the current clip from SD to the codec; paced by the blocking `esp_codec_dev_write` |

The SD card and the LCD share SPI2, so a mutex (`spi2_bus_lock`) serialises them and the LVGL flush
is synchronous.

### Memory (measured on the board)

| Item | Value |
|---|---|
| App image | 0x1e1a60 B ≈ 1.9 MiB of a 3 MiB OTA slot (37 % free) |
| Internal heap | ~137 KB free after LVGL init → **~41 KB steady**, ~39 KB while BLE-connected / playing |
| LVGL heap | 160 KB (`CONFIG_LV_MEM_SIZE_KILOBYTES`) |
| Draw buffer | one 466×100 DMA buffer (~93 KB) — 5 bands/frame |
| Flash layout | `nvs` 24 K · `otadata` · `phy_init` · `ota_0` 3 M · `ota_1` 3 M · `storage` 1 M (reserved) |

## 4. What is implemented

### UI (German strings, one dark theme)

| Screen / layer | Behaviour |
|---|---|
| **Boot** | free-road picture sweeps up the screen once (skippable via UiConfig) |
| **Presence** (0.2) | **Offline** (no app): black, crossed-out globe, only Settings · **Idle** (app connected, `driveActive = false`): the TNV logo in the middle, only Settings · **Driving**: everything below |
| **Home** | speed-limit sign (drawn, 3-digit fits the ring), free-road picture (replaced the green chevrons), globe + peer count (abbreviated "480K"), "+" button, gear. **Left column = prohibitions, right = danger signs; newest 3 per column** (oldest drops off) |
| **Hazard overlay** | per-category blinking icon (StVO sign where one fits, else a glyph — camera, radar, caravan, wrench, **car-crash**), distance bar, hazard speed limit replaces the sign; road picture/columns hide |
| **Confirm ("Gesehen?")** | scrim, Ja / Nein, countdown ring; navigation locked; silent timeout |
| **Melden** | rotary crown: 11 categories ride the rim, grab anywhere to spin with momentum, tap the hub to send; back via down-fling or rightward swipe |
| **Settings** | 2×2: Ton · Einheiten · **Zurücksetzen** (full factory reset: settings + cache + BLE bond, reboot to Connect) · **Updates** (info panel); footer shows version/proto/link |
| **Connect** | first-pairing screen with store QR + 3-step guide; shown until first bond |
| **OTA** | full-screen "Update NN %" progress layer while a firmware image streams in |

Home ↔ Melden transitions triggered programmatically **cut** instead of sliding (see §7). Details
and pixel specs: [`ui-spec.md`](ui-spec.md); renders: [`screenshots/`](screenshots/).

### BLE protocol (service base `079aXXXX-d673-469c-98a0-12c0c5eb43b3`, **frozen for v1**)

| Slot | Name | Props | Purpose |
|---|---|---|---|
| 0001 | State | write-no-rsp | `DisplayState` (+ optional `audioCue`); dropped if `stateSeq` ≤ last |
| 0002 | Event | notify | `ReportPressed/Submitted`, `ConfirmResponse`, settings changes, `Heartbeat` |
| 0003 | DeviceInfo | read | proto/fw version, capability bits (audio, rtc, sd, speedcam, ota), resolution |
| 0004 | Settings | read/write/notify | sound on/mode/**volume 0–100**, theme (ignored), mph, hazard filter bits, night hours, cache stale minutes. NVS-persisted |
| 0005 | Time | write | epoch + tz → PCF85063 RTC |
| 0006 | UiConfig | read/write/notify | 10 persistent dev feature switches: Melden page, Settings page, hazard overlay, confirm overlay, sign columns, direction arrows, peer count, gear, connect QR, boot animation |
| 0007 | OTA | write-no-rsp/notify | raw-framed firmware image → dual-slot OTA, SHA-256 verify, boot-rollback watchdog |
| 0008 | Asset | write-no-rsp/notify | copy files onto the SD card (`BEGIN path` / `DATA` / `END`) |

Wire tables, example bytes, compatibility rule: [`ble-protocol.md`](ble-protocol.md). Bonding is
persisted in NVS so a phone re-encrypts silently each ignition cycle.

### Audio

The app sends an `AudioCue` (mode tones/voice, event, hazard type, distance, speed). The device
composes the playlist (`core/audio/cue_composer`): a tone chime, or **voice** = "Achtung" + hazard
type + "in" + distance bucket (100/200/300/500/800 m, rounded down). Clips are 16 kHz mono 16-bit
WAV **streamed from SD in 1 KB chunks** (loading a clip whole ran out of RAM); `warn`/`info` chimes
are also embedded in flash as a no-SD fallback. The master gate is `Settings.soundEnabled`;
volume is app-adjustable and applied live to the codec. Voice clips are **German Windows-SAPI TTS
placeholders that are not distributed** (redistribution terms unclear): [`tools/gen_voice.py`](../tools/gen_voice.py)
generates and uploads them locally. Without them a voice cue degrades to the event chime
([`audio-assets.md`](audio-assets.md)).

### Persistence & resilience

- NVS: Settings (blob format v2), UiConfig, offline cache of the last `DisplayState` (written every
  ~30 s; restored on boot so Home shows the last-known limit instead of the pairing screen), BLE bonds.
- A quiet BLE link downgrades to "advertising" after a watchdog → cache + "no app" badge.
- Reconnect clears the stale `stateSeq` high-water mark (an earlier bug froze the UI after an app restart).
- OTA rolls back automatically if the new image doesn't confirm itself (~3 s after boot).

### Tooling

| Tool | Purpose |
|---|---|
| `tools/fake_app.py` | Windows/PC BLE test app. **GUI (default, tkinter):** *Dev — vollständig* tab (every DisplayState field, hazard/confirm/audio controls, sign picker with automatic left/right placement and mutually-exclusive sign families, feature switches, settings incl. volume, time, OTA, scenarios) and *App-Simulation* tab (what the future phone app would show; user-level "which hazards to warn about" filter). Also `--repl`, `--scenario`, `--all`, `--dry`, `--scan` |
| `tools/gen_voice.py` | generate + upload the voice clips |
| `tools/gen_assets.sh`, `gen_asset_catalog.py` | SVG → LVGL C arrays + catalogs (signs, icons) |
| `tools/gen_fonts.sh`, `gen_audio_tones.py`, `build_sd_audio.sh` | fonts, synthetic tones, SD layout |
| `sim/bdsp_sim` | desktop simulator (`--shot` writes `docs/screenshots/*.png`, `--demo` plays the mock timeline) |

## 5. Milestone history

| | Milestone | State |
|---|---|---|
| M0–M1 | research, skeleton, host build | ✅ |
| M2 / M2.5 | design system, Home, simulator · sign & icon asset library | ✅ |
| M3–M5 | navigation + tiles · overlays · state machine + mock/demo | ✅ |
| M6 | audio pipeline (composer, service, fallback tones) | ✅ |
| M7 | CBOR codec, BLE protocol, NimBLE transport | ✅ |
| M8 | CI, README, docs | ✅ |
| Phase B / D | display + touch + full UI on hardware · NimBLE peripheral on hardware | ✅ |
| M9-A | UiConfig characteristic + feature switches | ✅ |
| M9-B | microSD over the shared SPI2 bus | ✅ |
| M9-C | ES8311 audio (tones + composed voice), app-adjustable volume | ✅ |
| M9-D | OTA over BLE, dual-slot partitions, rollback | ✅ built |
| M9-E | tkinter GUI test console (two layers) | ✅ |
| Polish | factory reset + updates panel, accident glyph, newest-3 sign columns, audio underrun fix, tear mitigation | ✅ |

## 6. Verification status

**Verified on the real board**
- Boot, display, touch, full UI, all overlays; BLE State → UI, Settings both ways, Time → RTC, reconnect (2026-08-30).
- SD card mounts (14.6 GB); asset upload of the 23 audio files succeeded 23/23 over BLE.
- Audio plays in real time: a 6.58 s voice announcement ran 6.68 s wall-clock after the dedicated-task fix.
- Volume set to 20 / 90 / 55 over BLE reached the codec each time and read back correctly; persists across reboot.
- UiConfig **read** (`all on` on connect); a 4-sign column push and an Accident hazard are accepted without a crash.
- Heap steady at ~41 KB; single-buffer display change was RAM-neutral.

**Not yet verified / needs eyes or ears**
- OTA transfer end-to-end (the dual-slot table and self-confirm are live; no image has been pushed yet).
- UiConfig **write** + persistence round-trip from the GUI.
- Whether the audio crackle is gone subjectively (timing says the underrun is fixed; needs a listen).
- Whether the tear seam is acceptable on a fast *live* swipe into Melden (only the programmatic cut is guaranteed seam-free).
- The factory-reset flow (Settings → Zurücksetzen → reboot to Connect) is built and covered by the
  sim screenshots, but no on-device run of it is recorded in the checks above.

## 7. Known limitations

- **Tearing:** no TE line and partial rendering mean a fast full-screen slide shows a skewed seam. Mitigated (5 bands, cut on programmatic nav); a live finger-swipe still animates and can tear briefly.
- **Free RAM is tight** (~41 KB internal). Keep new features streaming / allocation-free; avoid growing the draw buffer.
- Voice clips are TTS placeholders; German only (i18n scaffold supports more).
- Theme field is accepted but ignored (single dark theme); `nightMode` only dims the panel.
- IMU and battery/power monitor are stubs; ES7210 microphone unused.

## 8. Open items

**Waiting on the owner**
- `BSP_PAIR_URL` — now the repository page; swap for a store link when the app is published.
- Repo naming and visibility decision (target repo `Romsmo/TN-DSP`). (`LICENSE` is MIT, © R. Smolenski.)
- Real voice recordings (or a decision to ship none) — the TTS placeholders are not published.
- **Security level of the BLE link** — today nothing requires pairing/encryption, so OTA and asset upload are open to any central in range (`ble-protocol.md`, *Security level*; options in `update-authenticity.md`).
- **Name and third-party references before the repo goes public** — the product name embeds a third-party brand; `docs/plan.md` mentions a licensing discussion with named parties; `BSP_PAIR_URL` / the pairing QR point at a third-party domain.
- Decided 2026-10-24: UUID base frozen, security level stays open for v1, no Melden lock, no voice clips shipped, Waveshare init table kept (documented).

**Verify on hardware**
- OTA cycle; UiConfig write round-trip; audible quality; swipe tear.
- Remaining `TODO(verify)` entries in `board_config.h` / `hardware.md`: brightness DCS command (`0x51`), TE line, TCA9554 P0–P6 map, battery ADC pin + divider, IMU address.

**Phone-app handover (out of repo scope, but the device depends on it)**
- The sign **left/right placement and "this replaces that" families** (e.g. Halteverbot 283/286, Überholverbot 276/277/280/281, 282 cancelling streckenverbote) currently live in `fake_app.py` (`SIGN_GROUPS`, `SIGN_CANCELS`, `sign_side`). The real app must reproduce them — the device only renders `leftSigns` / `rightSigns` as sent.
- The app owns hazard-category filtering (`filterBits`), the audio cue decision, volume, and OTA delivery.

**Doc debt** — the stale README status, bring-up table, ui-spec OTA note, board_config audio comment, M9 plan status and CI comment were fixed in the publication-readiness pass (2026-10-24).

**Release state:** `fw-v0.1.0` and `fw-v0.2.0` (image `tn-display-<v>.bin`, manifest, license bundle) and the plain tags `protocol-v1.0.0` / `protocol-v1.1.0` exist on GitHub. Branch protection is not available for a private repo on the free GitHub plan. The OTA transfer, UiConfig write, factory reset, an audible check and the new idle / offline screens on the AMOLED are still to be run on the board (`docs/testing.md`, *Board verification*).

## 9. Build & run cheat sheet

```bash
# host: tests + simulator (MSYS2/UCRT64 on Windows; -DBDSP_WERROR=ON as CI does)
cmake -S . -B build -G Ninja -DBDSP_WERROR=ON && cmake --build build
ctest --test-dir build --output-on-failure
./build/sim/bdsp_sim              # keys 1-9 scenarios, d demo, s shot ; --shot headless PNGs

# firmware (ESP-IDF 5.3.2, run from an IDF-exported PowerShell; board on COM4)
cd firmware && idf.py build && idf.py -p COM4 -b 921600 flash

# drive the device from the PC (Bluetooth radio must be on)
pip install bleak cbor2
python tools/fake_app.py                      # GUI
python tools/gen_voice.py --upload            # (re)generate + upload voice clips to the SD card
```

## 10. Document index

| Doc | Content |
|---|---|
| [`plan.md`](plan.md) | the original approved plan, milestones M0–M8, deviations forced by the C6 board |
| [`architecture.md`](architecture.md) | the render-never-compute rule, layers, threading |
| [`hardware.md`](hardware.md) | verified facts, pin map, risks |
| [`ui-spec.md`](ui-spec.md) | per-screen layout and behaviour |
| [`ble-protocol.md`](ble-protocol.md) | GATT service, every characteristic, CBOR keys |
| [`audio-assets.md`](audio-assets.md) | clip format, SD layout, composition rules, generation |
| [`firmware-bringup.md`](firmware-bringup.md) | build phases, HAL layout, bring-up notes |
| [`testing.md`](testing.md) | BLE test-console walkthrough and the on-device checklist |
| [`m9-completion-plan.md`](m9-completion-plan.md) | the post-plan completion milestones M9-A…E |
| [`building.md`](building.md) | host toolchain notes |
| [`app-feature-checklist.md`](app-feature-checklist.md) | every display function as a checklist for the app side (what to send, what to handle, what is missing) |
| [`app-integration.md`](app-integration.md) / [`.de`](app-integration.de.md) | handover to the app side: duties, capability bits, limitations, open decisions |
| [`contract/`](contract/README.md) | golden vectors, sign rules, responsibilities, UUID + report-lock proposals, manifest schema |
| [`release-format.md`](release-format.md), [`compatibility.md`](compatibility.md) | update manifest, version rules, firmware ↔ app table |
| [`update-authenticity.md`](update-authenticity.md) | signing / secure-boot options with irreversibility notes |
| [`legal-notice-draft.md`](legal-notice-draft.md) | draft legal notice for review |
