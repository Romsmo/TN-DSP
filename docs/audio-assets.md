# Audio assets

The board speaks warnings through its own speaker (ES8311 codec). The **app** decides *when* and
sends an `AudioCue`; the firmware decides *how it sounds* (`core/audio/cue_composer`) and plays it.

## Format

All clips: **WAV, 16-bit PCM, mono, 16 kHz**. No compression (streamed straight to the codec).

## Layout on the SD card

```
/audio/
  tones/
    warn.wav  alert.wav  confirm.wav  info.wav  clear.wav
  voice/
    de/
      achtung.wav  in.wav
      blitzer_fest.wav  blitzer_mobil.wav  blitzer_anhaenger.wav
      blitzer_rotlicht.wav  blitzer_abstand.wav
      gefahr_stau.wav  gefahr_glaette.wav  gefahr_unfall.wav
      gefahr_baustelle.wav  gefahr_panne.wav  gefahr_hindernis.wav
      d_100.wav  d_200.wav  d_300.wav  d_500.wav  d_800.wav
```

Paths and the clip vocabulary are defined in `core/audio/clips.h`. `warn.wav` and `info.wav` are
**also embedded in flash** (`core/audio/fallback_tones.cpp`) so a hazard chime still plays with no
card inserted.

## Composition

| Cue mode | Playlist |
|---|---|
| `Tones` | one event chime (`warn` / `alert` / `confirm` / `info` / `clear`) |
| `Voice`, hazard approach | `achtung` + `<type>` + `in` + `<nearest distance bucket>` |
| `Voice`, confirm prompt | `confirm` chime |

Distance buckets round **down**: 100 / 200 / 300 / 500 / 800 m.

## Which clips ship, which don't

- **Tones** (`tools/audio/tones/`) are synthesised by `tools/gen_audio_tones.py` — our own output,
  committed to the repository. `warn` and `info` are also compiled into the firmware.
- **Voice phrases are NOT in the repository.** The placeholder set was produced with the Windows
  SAPI German voice (*Microsoft Hedda*); whether its output may be redistributed is unclear, so
  it is not published. Generate it locally, or record your own — keep the filenames (the table
  above) and the 16 kHz / mono / 16-bit format.
- **Without the voice clips the device is still fine:** if any phrase of a voice playlist is
  missing on the card (or there is no card), the cue degrades to the event chime; a chime with no
  copy anywhere falls back to the built-in warn chime. A warning is never silent.

```bash
python tools/gen_audio_tones.py                       # tones + flash fallback C array
python tools/gen_voice.py                             # German TTS clips (Windows) + assemble tools/sd_root/
python tools/gen_voice.py --upload                    # ... and send them to the device over BLE
```

## On the device

The ES8311 codec + I2S output is `firmware/components/hal_esp/esp_audio.cpp`
(`CONFIG_BDSP_ENABLE_AUDIO`, on by default). Pins in `board_config.h`
(MCLK 19 / BCLK 21 / WS 22 / DOUT 23); the speaker amplifier is enabled through TCA9554 **P7**
when sound is on. `AudioService` streams each clip to the codec a chunk at a time from the io task.

To load the voice clips onto the card:

```bash
python tools/gen_voice.py              # regenerate the clips + assemble tools/sd_root/audio/
python tools/gen_voice.py --upload     # + push the tree to the device over BLE

# or, card in a reader:
cp -r tools/sd_root/*  <mounted SD card>/
```

`gen_voice.py` drives the Windows SAPI voices (`--voice Hedda`, `--list-voices`). The BLE push
uses the Asset characteristic (`docs/ble-protocol.md`); the GUI has buttons for it too.

`warn.wav` and `info.wav` are embedded in flash, so hazard/info chimes still play with **no card**;
everything else needs the card.

## Master gate

`Settings.soundEnabled` ("Ton An/Aus") gates all board audio. The toggle is also mirrored to the
app as `SettingsSoundToggle`. `Settings.soundMode` (tones/voice) is chosen in the app and travels
in each `AudioCue`.
