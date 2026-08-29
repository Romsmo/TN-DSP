# BlitzerdeDsp

Open-source firmware for a dedicated in-car AMOLED display: speed limits, hazard / speed-camera
warnings, context traffic signs, and a touch reporting UI. All live data arrives over Bluetooth LE
from a companion smartphone app — **the board never talks to an external API**. It renders the
state it receives and emits raw interaction events.

<p align="center">
  <img src="docs/screenshots/home.png" width="200" alt="Home">
  <img src="docs/screenshots/hazard.png" width="200" alt="Hazard">
  <img src="docs/screenshots/melden.png" width="200" alt="Melden">
  <img src="docs/screenshots/confirm.png" width="200" alt="Confirm">
</p>

- **Target board:** Waveshare **ESP32-C6-Touch-AMOLED-1.43** (466×466 round AMOLED, no PSRAM). See
  [`docs/hardware.md`](docs/hardware.md).
- **Stack:** ESP-IDF ≥ 5.3, LVGL 9, C++17, NimBLE, hand-rolled CBOR.
- **Status:** M0–M7 complete (host-testable core + UI + simulator + protocol). On-target bring-up
  (display / touch / BLE flashing) pending an ESP-IDF environment — the firmware layer is written
  but not yet compiled against IDF.

> The companion **Flutter app is not in this repo**. This repo defines the BLE protocol
> ([`docs/ble-protocol.md`](docs/ble-protocol.md)), a Python stand-in ([`tools/fake_app.py`](tools/fake_app.py)),
> and a Dart reference ([`docs/app-reference/`](docs/app-reference/)).

## Repository layout

| Path | Contents |
|---|---|
| `core/` | Platform-independent logic — model, state machine, CBOR codec, cache, settings, audio. Host-compilable, LVGL-free. |
| `ui/` | LVGL screens, widgets, theme, i18n, generated font/sign/icon assets. No IDF/SDL includes. |
| `hal/` | Hardware abstraction interfaces. |
| `transport/` | `ITransport` + Mock (demo/sim). |
| `sim/` | Desktop build of `ui/` + `core/` against LVGL-SDL — the fast iteration path. |
| `test/` | Host unit tests (CTest, doctest). |
| `firmware/` | The ESP-IDF application + ESP-specific HAL / BLE transport. |
| `tools/` | Asset pipelines (fonts, signs, icons, audio), `fake_app.py`. |
| `docs/` | `plan.md`, `hardware.md`, `architecture.md`, `ui-spec.md`, `ble-protocol.md`, `audio-assets.md`. |

Read [`docs/architecture.md`](docs/architecture.md) first — the guiding rule is *the display
renders, it never computes*.

## Build — host (tests + simulator), no hardware

Needs a C++17 compiler, CMake ≥ 3.20, Ninja, and SDL2 (simulator only).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the simulator (mouse = touch):

```bash
./build/sim/bdsp_sim            # keys 1-7 = scenarios, d = demo timeline, s = screenshot
./build/sim/bdsp_sim --demo     # start in the mock demo (audible warnings)
./build/sim/bdsp_sim --shot     # headless: write docs/screenshots/*.png
```

Platform notes for the host toolchain (incl. an antivirus-TLS workaround for Windows/MSYS2) are in
[`docs/building.md`](docs/building.md).

## Build — firmware

Needs ESP-IDF ≥ 5.3.

```bash
cd firmware
idf.py set-target esp32c6
idf.py build flash monitor
```

Feature flags are Kconfig options under **BlitzerdeDsp** (`idf.py menuconfig`), notably
`CONFIG_BDSP_FEATURE_SPEEDCAM` — turn it off for a build with no speed-camera code at all
(§ 23 (1b) StVO).

## Assets

All regenerable; the generated outputs are committed so a plain `cmake` build needs no extra tools.

| Asset | Script | Source |
|---|---|---|
| Montserrat fonts | `tools/gen_fonts.sh` | JulietaUla/Montserrat (OFL) |
| Traffic signs | `tools/signs/*` + `tools/gen_assets.sh` | Wikimedia Commons German StVO (public domain) |
| UI icons | `tools/gen_assets.sh` | Lucide (ISC) |
| Audio | `tools/gen_audio_tones.py`, `tools/gen_audio_voice.ps1`, `tools/build_sd_audio.sh` | synth tones + SAPI TTS (placeholders) |

Copy `tools/sd_root/` onto the microSD card for on-device audio. Replace `tools/audio/voice/de/*.wav`
with real recordings (same filenames, 16 kHz mono 16-bit) — see [`docs/audio-assets.md`](docs/audio-assets.md).

## License

MIT — see [`LICENSE`](LICENSE). Third-party asset licenses: [`third_party/README.md`](third_party/README.md).
