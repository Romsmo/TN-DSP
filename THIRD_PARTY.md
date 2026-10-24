# Third-party material

Everything this repository contains or builds on that it did not write itself, with source,
license and date checked. **Draft for a lawyer's review — not legal advice.** Anything that could
not be backed up is not in the repository (see "Not distributed").

Project code: MIT, © 2026 R. Smolenski (see [`LICENSE`](LICENSE)). The BLE protocol, the CBOR codec
(`core/codec/`) and all UI code are original work.

## 1. Code vendored in the repository

| Item | Version | License | Where | Notes |
|---|---|---|---|---|
| [doctest](https://github.com/doctest/doctest) | 2.4.11 | MIT (© 2016-2023 Viktor Kirilov) | `third_party/doctest/` | host unit tests only; not in the firmware |
| [stb_image_write](https://github.com/nothings/stb) | 1.16 | MIT **or** public domain (Unlicense), your choice — text at the end of the header | `third_party/stb/` | simulator PNG export only; not in the firmware |
| `LVGLImage.py` | from LVGL 9.2 | MIT (© LVGL Kft) | `tools/LVGLImage.py` | build-time asset conversion |

## 2. Firmware dependencies (fetched at build time)

Versions from `firmware/dependencies.lock` / the ESP-IDF 5.3.2 tree, licenses read from the
shipped license files on 2026-10-24.

| Component | Version | License | Linked into the firmware image |
|---|---|---|---|
| [ESP-IDF](https://github.com/espressif/esp-idf) (FreeRTOS, drivers, newlib glue, …) | 5.3.2 | Apache-2.0 (components keep their own notices) | yes |
| NimBLE host/controller glue (in ESP-IDF, from Apache Mynewt) | IDF 5.3.2 | Apache-2.0 | yes |
| mbedTLS (SHA-256 for OTA verification) | IDF 5.3.2 | Apache-2.0 (dual Apache-2.0 / GPL-2.0-or-later; we use it under Apache-2.0) | yes |
| FatFs (ChaN) | R0.15 w/patch2 (in IDF) | BSD-style (© 2022 ChaN) — redistribution requires keeping the notice | yes |
| [lvgl/lvgl](https://github.com/lvgl/lvgl) | 9.2.2 | MIT (© 2021 LVGL Kft) | yes |
| QR code generator (bundled in LVGL, `lv_qrcode`) | LVGL 9.2.2 | MIT (© Project Nayuki) | yes |
| [espressif/esp_lcd_sh8601](https://components.espressif.com/components/espressif/esp_lcd_sh8601) | 2.0.1~1 | Apache-2.0 | yes |
| [espressif/esp_codec_dev](https://components.espressif.com/components/espressif/esp_codec_dev) | 1.3.6 | Apache-2.0 | yes |
| espressif/cmake_utilities | 0.5.3 | Apache-2.0 | build helper |

**Binary releases** (the OTA image) contain code under Apache-2.0, MIT and the FatFs terms. A
release must therefore ship this file plus the license texts of those components alongside the
image. `TODO(verify)`: assemble that `licenses/` bundle in the release job (see the release plan).

The host simulator additionally uses LVGL (MIT) and SDL2 (zlib license), as installed on the
developer's machine.

## 3. Assets

### Traffic signs — `ui/assets/signs/`, sources in `tools/signs/svg/` (58 files)

German road signs (StVO), SVG from Wikimedia Commons. **Status per file** as reported by the
Commons metadata API on 2026-10-24: every one of the 58 files is marked *public domain*,
*not copyrighted*, *no attribution required*, with the license templates shown below
(`PD-GermanGov` = official works of German authorities, § 5 UrhG; `PD-VzKat` / `PD-Vz historisch`
= Verkehrszeichen-Katalog / historical sign designs). That is Commons' own volunteer-maintained
assessment, not a legal opinion; the project relies on it. Source URLs for re-checking are in
`tools/signs/manifest.json`.

| VzNr | Commons file | Public-domain templates on the file page |
|---|---|---|
| 101 | [Zeichen 101 - Gefahrstelle, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101_-_Gefahrstelle,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 101-10 | [Zeichen 101-10 - Flugbetrieb, Aufstellung rechts, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-10_-_Flugbetrieb,_Aufstellung_rechts,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-11 | [Zeichen 101-11 - Fußgängerüberweg, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-11_-_Fu%C3%9Fg%C3%A4nger%C3%BCberweg,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-12 | [Zeichen 101-12 - Viehtrieb, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-12_-_Viehtrieb,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-13 | [Zeichen 101-13 - Reiter, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-13_-_Reiter,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-14 | [Zeichen 101-14 - Amphibienwanderung, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-14_-_Amphibienwanderung,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-15 | [Zeichen 101-15 - Steinschlag, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-15_-_Steinschlag,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-51 | [Zeichen 101-51 - Schnee- oder Eisglätte, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-51_-_Schnee-_oder_Eisgl%C3%A4tte,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-52 | [Zeichen 101-52 - Splitt, Schotter, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-52_-_Splitt,_Schotter,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-53 | [Zeichen 101-53 - Ufer, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-53_-_Ufer,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-54 | [Zeichen 101-54 - Unzureichendes Lichtraumprofil, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-54_-_Unzureichendes_Lichtraumprofil,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 101-55 | [Zeichen 101-55 - Bewegliche Brücke, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_101-55_-_Bewegliche_Br%C3%BCcke,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 102 | [Zeichen 102 - Kreuzung oder Einmündung mit Vorfahrt von rechts, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_102_-_Kreuzung_oder_Einm%C3%BCndung_mit_Vorfahrt_von_rechts,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 103-10 | [Zeichen 103-10 - Kurve (links), StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_103-10_-_Kurve_(links),_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 105-10 | [Zeichen 105-10 - Doppelkurve (zunächst links), StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_105-10_-_Doppelkurve_(zun%C3%A4chst_links),_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 108-4 | [Zeichen 108-4 - Gefälle, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_108-4_-_Gef%C3%A4lle,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 110-4 | [Zeichen 110-4 - Steigung, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_110-4_-_Steigung,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 112 | [Zeichen 112 – Unebene Fahrbahn, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_112_%E2%80%93_Unebene_Fahrbahn,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 114 | [Zeichen 114 - Schleuder- oder Rutschgefahr, StVO 2013.svg](https://commons.wikimedia.org/wiki/File:Zeichen_114_-_Schleuder-_oder_Rutschgefahr,_StVO_2013.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 117-10 | [Zeichen 117-10 - Seitenwind von rechts, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_117-10_-_Seitenwind_von_rechts,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 120 | [Zeichen 120 - Verengte Fahrbahn, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_120_-_Verengte_Fahrbahn,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 121-10 | [Zeichen 121-10 - einseitig (rechts) verengte Fahrbahn, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_121-10_-_einseitig_(rechts)_verengte_Fahrbahn,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 123 | [Zeichen 123 - Arbeitsstelle, StVO 2013.svg](https://commons.wikimedia.org/wiki/File:Zeichen_123_-_Arbeitsstelle,_StVO_2013.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 124 | [Zeichen 124 - Stau, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_124_-_Stau,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 125 | [Zeichen 125 - Gegenverkehr, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_125_-_Gegenverkehr,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 131 | [Zeichen 131 - Lichtzeichenanlage, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_131_-_Lichtzeichenanlage,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 133-10 | [Zeichen 133-10 - Fußgänger (Aufstellung rechts), StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_133-10_-_Fu%C3%9Fg%C3%A4nger_(Aufstellung_rechts),_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 136-10 | [Zeichen 136-10 - Kinder, Aufstellung rechts, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_136-10_-_Kinder,_Aufstellung_rechts,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 138-10 | [Zeichen 138-10 - Radverkehr, StVO 2013.svg](https://commons.wikimedia.org/wiki/File:Zeichen_138-10_-_Radverkehr,_StVO_2013.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 142-10 | [Zeichen 142-10 - Wildwechsel, Aufstellung rechts, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_142-10_-_Wildwechsel,_Aufstellung_rechts,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 151 | [Zeichen 151 - Bahnübergang, StVO 2013.svg](https://commons.wikimedia.org/wiki/File:Zeichen_151_-_Bahn%C3%BCbergang,_StVO_2013.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 250 | [Zeichen 250 - Verbot für Fahrzeuge aller Art, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_250_-_Verbot_f%C3%BCr_Fahrzeuge_aller_Art,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 251 | [Zeichen 251 - Verbot für Kraftwagen und sonstige mehrspurige Kraftfahrzeuge, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_251_-_Verbot_f%C3%BCr_Kraftwagen_und_sonstige_mehrspurige_Kraftfahrzeuge,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 253 | [Zeichen 253 - Verbot für Kraftfahrzeuge mit einem zulässigen Gesamtgewicht, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_253_-_Verbot_f%C3%BCr_Kraftfahrzeuge_mit_einem_zul%C3%A4ssigen_Gesamtgewicht,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 254 | [Zeichen 254 - Verbot für Radfahrer, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_254_-_Verbot_f%C3%BCr_Radfahrer,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 255 | [Zeichen 255 - Verbot für Krafträder, auch mit Beiwagen, Kleinkrafträder und Mofas, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_255_-_Verbot_f%C3%BCr_Kraftr%C3%A4der,_auch_mit_Beiwagen,_Kleinkraftr%C3%A4der_und_Mofas,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 257-50 | [Zeichen 257-50 - Verbot für Mofas, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_257-50_-_Verbot_f%C3%BCr_Mofas,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 259 | [Zeichen 259 - Verbot für Fußgänger, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_259_-_Verbot_f%C3%BCr_Fu%C3%9Fg%C3%A4nger,_StVO_1992.svg) | PD-GermanGov, PD-Layout |
| 260 | [Zeichen 260 - Verbot für Krafträder und Mofas und sonstige mehrspurige Kraftfahrzeuge, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_260_-_Verbot_f%C3%BCr_Kraftr%C3%A4der_und_Mofas_und_sonstige_mehrspurige_Kraftfahrzeuge,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 261 | [Zeichen 261 - Verbot für kennzeichnungspflichtige Kraftfahrzeuge mit gefährlichen Gütern, StVO 1988.svg](https://commons.wikimedia.org/wiki/File:Zeichen_261_-_Verbot_f%C3%BCr_kennzeichnungspflichtige_Kraftfahrzeuge_mit_gef%C3%A4hrlichen_G%C3%BCtern,_StVO_1988.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 262-5 | [Zeichen 262-5,5 - Verbot für Fahrzeuge über angegebene tatsächliche Masse, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_262-5,5_-_Verbot_f%C3%BCr_Fahrzeuge_%C3%BCber_angegebene_tats%C3%A4chliche_Masse,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 263-8 | [Zeichen 263-8 - Verbot für Fahrzeuge über angegebene tatsächliche Achslast, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_263-8_-_Verbot_f%C3%BCr_Fahrzeuge_%C3%BCber_angegebene_tats%C3%A4chliche_Achslast,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 264-2 | [Zeichen 264-2 - Verbot für Fahrzeuge über angegebene tatsächliche Breite, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_264-2_-_Verbot_f%C3%BCr_Fahrzeuge_%C3%BCber_angegebene_tats%C3%A4chliche_Breite,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 265-3 | [Zeichen 265-3,8 - Verbot für Fahrzeuge über angegebene tatsächliche Höhe, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_265-3,8_-_Verbot_f%C3%BCr_Fahrzeuge_%C3%BCber_angegebene_tats%C3%A4chliche_H%C3%B6he,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 266-10 | [Zeichen 266-10 - Verbot für Fahrzeuge über angegebene tatsächliche Länge, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_266-10_-_Verbot_f%C3%BCr_Fahrzeuge_%C3%BCber_angegebene_tats%C3%A4chliche_L%C3%A4nge,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 267 | [Zeichen 267 - Verbot der Einfahrt, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_267_-_Verbot_der_Einfahrt,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 268 | [Zeichen 268 - Schneeketten sind vorgeschrieben, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_268_-_Schneeketten_sind_vorgeschrieben,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 269 | [Zeichen 269 - Verbot für Fahrzeuge mit wassergefährdender Ladung, StVO 1988.svg](https://commons.wikimedia.org/wiki/File:Zeichen_269_-_Verbot_f%C3%BCr_Fahrzeuge_mit_wassergef%C3%A4hrdender_Ladung,_StVO_1988.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 270 | [Zeichen 270.1 - Beginn eines Verkehrsverbots zur Verminderung schädlicher Luftverunreinigungen in einer Zone. I, StVO 2007.svg](https://commons.wikimedia.org/wiki/File:Zeichen_270.1_-_Beginn_eines_Verkehrsverbots_zur_Verminderung_sch%C3%A4dlicher_Luftverunreinigungen_in_einer_Zone._I,_StVO_2007.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 272 | [Zeichen 272, Verbot des Wendens, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_272,_Verbot_des_Wendens,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 273-70 | [Zeichen 273-70 - Verbot des Unterschreitens des angegebenen Mindestabstandes, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_273-70_-_Verbot_des_Unterschreitens_des_angegebenen_Mindestabstandes,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 276 | [Zeichen 276 - Überholverbot für Kraftfahrzeuge aller Art, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_276_-_%C3%9Cberholverbot_f%C3%BCr_Kraftfahrzeuge_aller_Art,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 277 | [Zeichen 277 - Überholverbot für Kraftfahrzeuge mit einem zulässigen Gesamtgewicht über 2,8 t, einschließlich ihrer Anhänger, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_277_-_%C3%9Cberholverbot_f%C3%BCr_Kraftfahrzeuge_mit_einem_zul%C3%A4ssigen_Gesamtgewicht_%C3%BCber_2,8_t,_einschlie%C3%9Flich_ihrer_Anh%C3%A4nger,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 280 | [Zeichen 280 - Ende des Überholverbotes für Kraftfahrzeuge aller Art, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_280_-_Ende_des_%C3%9Cberholverbotes_f%C3%BCr_Kraftfahrzeuge_aller_Art,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 281 | [Zeichen 281 - Ende des Überholverbots für Kraftfahrzeuge mit einem zulässigen Gesamtgewicht über 3,5 t, StVO 1992.svg](https://commons.wikimedia.org/wiki/File:Zeichen_281_-_Ende_des_%C3%9Cberholverbots_f%C3%BCr_Kraftfahrzeuge_mit_einem_zul%C3%A4ssigen_Gesamtgewicht_%C3%BCber_3,5_t,_StVO_1992.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 282 | [Zeichen 282 - Ende sämtlicher Streckenverbote, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_282_-_Ende_s%C3%A4mtlicher_Streckenverbote,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |
| 283 | [Zeichen 283 - Absolutes Haltverbot, StVO 2017.svg](https://commons.wikimedia.org/wiki/File:Zeichen_283_-_Absolutes_Haltverbot,_StVO_2017.svg) | PD-GermanGov, PD-Layout, PD-VzKat |
| 286 | [Zeichen 286 - Eingeschränktes Halteverbot, StVO 1970.svg](https://commons.wikimedia.org/wiki/File:Zeichen_286_-_Eingeschr%C3%A4nktes_Halteverbot,_StVO_1970.svg) | PD-GermanGov, PD-Layout, PD-Vz historisch |

The sign *shapes* are official traffic signs; showing them on a display does not make this product
an official traffic-sign device.

### Icons — `ui/assets/icons/`, sources in `tools/icons/svg/` (22 from Lucide + 1 own)

[Lucide](https://lucide.dev) icons, ISC license (© Lucide Icons and Contributors), with the
portions derived from Feather under the MIT license (© Cole Bemis). Both license texts are in
[`tools/icons/LICENSE`](tools/icons/LICENSE) and must accompany redistributions. The car-crash
icon (`tools/icons/svg/car-crash.svg`) is original work of this project (MIT), drawn in Lucide's
style from Lucide's `car` outline plus four strokes.

### Font — `ui/theme/fonts/`, `tools/fonts/ttf/`

[Montserrat](https://github.com/JulietaUla/Montserrat), SIL Open Font License 1.1
(© 2024 The Montserrat.Git Project Authors; no Reserved Font Name is declared in the copyright
line). License text: [`ui/theme/fonts/OFL.txt`](ui/theme/fonts/OFL.txt). The LVGL C files are
subsetted conversions (Modified Version under the OFL); the license text travels with them.

### Audio

| Item | Origin | Status |
|---|---|---|
| Event chimes (`tools/audio/tones/`, `core/audio/fallback_tones.cpp`) | synthesised by `tools/gen_audio_tones.py` | original work, MIT |
| Voice phrases (`/audio/voice/de/*.wav`) | placeholders from the Windows SAPI German voice | **not distributed** — see below |

## 4. Hardware reference (no code copied)

Pin numbers, the touch controller's register addresses and a 7-entry panel initialisation table
were taken from Waveshare's example repository
[`waveshareteam/ESP32-C6-Touch-AMOLED-1.43`](https://github.com/waveshareteam/ESP32-C6-Touch-AMOLED-1.43).
**That repository carries no license** (GitHub API: `license: null`, checked 2026-10-24). Pin
assignments and register addresses are facts about the hardware; the firmware's drivers are
written independently. The init table in `firmware/components/hal_esp/esp_display.cpp`
(`kInit[]`, seven register writes) is the one piece that mirrors Waveshare's example.
Decision 2026-10-24: kept as is (the easiest option); it is seven register writes that the panel
needs, listed here so a reader can judge it. `TODO(verify)`: replace them with values from the CO5300 /
SH8601 datasheet, or ask Waveshare for permission, before a *public* release.

## 5. Tooling (not distributed with the firmware)

| Tool | License | Used for |
|---|---|---|
| cbor2 | MIT | `tools/fake_app.py` |
| bleak | MIT | `tools/fake_app.py` |
| Pillow | HPND | asset conversion |
| pypng | MIT | asset conversion |
| librsvg (`rsvg-convert`) | LGPL-2.1+ | SVG rasterisation (used as an external tool) |
| lv_font_conv 1.5.3 | MIT | font conversion |
| tkinter | PSF license (ships with Python) | test-app GUI |

## 6. Not distributed

- **Voice clips** (decision 2026-10-24: ship none). The German phrase clips were generated with the Windows SAPI voice
  *Microsoft Hedda*. Whether audio produced that way may be redistributed in an open-source
  repository could not be confirmed, so the files are not published. `tools/gen_voice.py`
  regenerates them locally; the firmware degrades to its event chime without them.
- Nothing else was excluded: every other asset above has a license text or a Commons status on
  record.

_Last audited: 2026-10-24._
