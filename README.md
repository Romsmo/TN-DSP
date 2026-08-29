# BlitzerdeDsp

Open-source firmware for a dedicated in-car AMOLED display: speed limits, hazard / speed-camera
warnings, and a touch reporting UI. All live data arrives over Bluetooth LE from a companion
smartphone app — **the board never talks to an external API**. It renders the state it receives
and emits raw interaction events.

- **Target board:** Waveshare **ESP32-S3-Touch-AMOLED-1.43** — *the C6 variant, see [`docs/hardware.md`](docs/hardware.md)*
- **Framework:** ESP-IDF ≥ 5.3, LVGL 9, C++17, NimBLE
- **Status:** in development — see the milestone plan below

> The companion Flutter app is **not** part of this repository yet. This repo defines the BLE
> protocol (`docs/ble-protocol.md`), ships a Python simulator of the app (`tools/fake_app.py`),
> and a Dart reference snippet (`docs/app-reference/`).

## Repository layout

| Path | Contents |
|---|---|
| `core/` | Platform-independent logic (model, state machine, codec, cache, settings). Host-compilable. |
| `ui/` | LVGL screens, widgets, theme, i18n. No IDF/Arduino includes. |
| `hal/` | Hardware abstraction interfaces. |
| `transport/` | `ITransport` + Mock / Serial implementations. |
| `sim/` | Desktop build of `ui/` + `core/` against LVGL-SDL. |
| `test/` | Host unit tests (CTest). |
| `firmware/` | The ESP-IDF application and ESP-specific HAL / BLE transport. |
| `tools/` | `fake_app.py` and asset tooling. |
| `docs/` | `plan.md`, `hardware.md`, `architecture.md`, `ui-spec.md`, `ble-protocol.md`. |

## Building

### Host (unit tests + simulator) — no ESP-IDF needed

Requires a C++17 compiler, CMake ≥ 3.20, Ninja, and SDL2 (simulator only).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### Firmware — ESP-IDF ≥ 5.3, target `esp32c6`

```bash
cd firmware
idf.py set-target esp32c6
idf.py build flash monitor
```

## Milestones

M0 research · **M1 skeleton & bring-up** · M2 design system + Home + simulator ·
M3 navigation + tile screens · M4 overlays · M5 core logic + demo/mock · M6 audio ·
M7 BLE transport + protocol · M8 simulator polish, CI, docs, license.

## License

MIT — see [`LICENSE`](LICENSE). *(Copyright holder: TBD.)*
