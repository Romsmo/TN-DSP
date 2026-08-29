# Vendored third-party code

| Library | Version | License | Use |
|---|---|---|---|
| [doctest](https://github.com/doctest/doctest) | 2.4.11 | MIT | host unit-test framework (`test/`) |
| [stb_image_write](https://github.com/nothings/stb) | 1.16 | public domain | simulator PNG screenshot export |
| [LVGLImage.py](https://github.com/lvgl/lvgl) | (from LVGL 9.2) | MIT | PNG → LVGL C array (`tools/`) |

Asset sources (not under `third_party/`):

| Asset | Source | License |
|---|---|---|
| Traffic signs (`ui/assets/signs/`) | Wikimedia Commons — German StVO | public domain (§ 5 UrhG) |
| UI icons (`ui/assets/icons/`) | [Lucide](https://lucide.dev) | ISC (`tools/icons/LICENSE`) |
| Montserrat (`ui/theme/fonts/`) | [JulietaUla/Montserrat](https://github.com/JulietaUla/Montserrat) | SIL OFL (`ui/theme/fonts/OFL.txt`) |

`tinycbor` (CBOR codec) is pulled as an ESP-IDF managed component for the firmware and
will be added here as a host copy for the shared codec at milestone M7.
