# Building

Two independent builds: the **host build** (unit tests + simulator, no hardware) and the
**firmware build** (ESP-IDF, flashes the board).

## Host build

### Requirements

- C++17 compiler (GCC ≥ 11 / Clang ≥ 14 / MSVC 19.3x)
- CMake ≥ 3.20, Ninja
- SDL2 (simulator only — tests build without it)

### Windows (MSYS2 / UCRT64)

```bash
# one-time
winget install MSYS2.MSYS2
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja \
                   mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-SDL2 \
                   mingw-w64-ucrt-x86_64-pkgconf
```

Add `C:\msys64\ucrt64\bin` to `PATH`.

> **TLS note.** If this machine runs antivirus that inspects HTTPS (Norton, Kaspersky, some
> corporate proxies), `pacman` / `curl` inside MSYS2 will fail certificate validation. Export the
> Windows root store to `C:\msys64\etc\pki\ca-trust\source\anchors\windows-roots.pem` and run
> `update-ca-trust`. For git, `git config --global http.sslBackend schannel`.

### Configure / build / test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Options: `-DBDSP_BUILD_SIM=OFF`, `-DBDSP_BUILD_TESTS=OFF`, `-DBDSP_WERROR=ON` (CI uses `WERROR`).

### Linux / macOS

```bash
sudo apt install build-essential cmake ninja-build libsdl2-dev   # Debian/Ubuntu
brew install cmake ninja sdl2                                    # macOS
```

## Firmware build

### Requirements

- ESP-IDF ≥ 5.3 (`esp32c6` target support)

### Build

```bash
cd firmware
idf.py set-target esp32c6
idf.py build
idf.py -p <PORT> flash monitor
```

Feature flags are Kconfig options under **BlitzerdeDsp** (`idf.py menuconfig`), e.g.
`CONFIG_BDSP_FEATURE_SPEEDCAM` (see [`docs/architecture.md`](architecture.md)).

## Repository is not yet on ESP-IDF here

ESP-IDF is **not installed** in the current dev environment — firmware milestones that need
on-target verification (display bring-up, flashing) are pending an ESP-IDF install. Host-testable
work (core logic, codec, state machine, simulator) proceeds independently.
