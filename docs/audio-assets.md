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

## Generating the placeholder set

The clips in `tools/audio/` are **placeholders** — synthetic tones plus Microsoft-SAPI German TTS
(*Microsoft Hedda*). Replace `voice/de/*.wav` with real recordings when available; keep the
filenames and the 16 kHz/mono/16-bit format.

```bash
python  tools/gen_audio_tones.py                      # tones + flash fallback C array
powershell -File tools/gen_audio_voice.ps1            # German TTS voice clips  (Windows)
tools/build_sd_audio.sh                               # assemble sd_root/audio/ for copying to a card
```

## On the device

The ES8311 codec + I2S output is `firmware/components/hal_esp/esp_audio.cpp`
(`CONFIG_BDSP_ENABLE_AUDIO`, on by default). Pins in `board_config.h`
(MCLK 19 / BCLK 21 / WS 22 / DOUT 23); the speaker amplifier is enabled through TCA9554 **P7**
when sound is on. `AudioService` streams each clip to the codec a chunk at a time from the io task.

To load the voice clips, copy the built tree onto the card:

```bash
tools/build_sd_audio.sh                # -> tools/sd_root/audio/
cp -r tools/sd_root/*  <mounted SD card>/
```

`warn.wav` and `info.wav` are embedded in flash, so hazard/info chimes still play with **no card**;
everything else needs the card.

## Master gate

`Settings.soundEnabled` ("Ton An/Aus") gates all board audio. The toggle is also mirrored to the
app as `SettingsSoundToggle`. `Settings.soundMode` (tones/voice) is chosen in the app and travels
in each `AudioCue`.
