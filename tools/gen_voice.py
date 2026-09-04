#!/usr/bin/env python3
"""gen_voice.py — (re)generate the German voice-announcement clips.

The board composes an announcement from short clips (core/audio/cue_composer):
"Achtung" + <hazard type> + "in" + <distance bucket>. This produces every clip
listed in core/audio/clips.h as 16 kHz / mono / 16-bit WAV, then assembles the
SD-card tree in tools/sd_root/.

On Windows it drives the built-in SAPI voices (no pip install). Pass --voice to
pick one; `--list-voices` shows what's installed. Replace the results with real
recordings later — keep the file names and the format.

    python tools/gen_voice.py                    # generate + assemble sd_root
    python tools/gen_voice.py --voice Hedda
    python tools/gen_voice.py --upload           # + push to the device over BLE
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VOICE_DIR = ROOT / "tools/audio/voice/de"
TONES_DIR = ROOT / "tools/audio/tones"
SD_AUDIO = ROOT / "tools/sd_root/audio"

# file stem -> spoken text. Stems must match core/audio/clips.h clip_path().
PHRASES = {
    "achtung":           "Achtung.",
    "in":                "in",
    "blitzer_fest":      "fester Blitzer",
    "blitzer_mobil":     "mobiler Blitzer",
    "blitzer_anhaenger": "Blitzeranhänger",
    "blitzer_rotlicht":  "Rotlichtblitzer",
    "blitzer_abstand":   "Abstandsmessung",
    "gefahr_stau":       "Stau",
    "gefahr_glaette":    "Glätte",
    "gefahr_unfall":     "Unfall",
    "gefahr_baustelle":  "Baustelle",
    "gefahr_panne":      "Panne",
    "gefahr_hindernis":  "Hindernis auf der Fahrbahn",
    "d_100":             "einhundert Metern",
    "d_200":             "zweihundert Metern",
    "d_300":             "dreihundert Metern",
    "d_500":             "fünfhundert Metern",
    "d_800":             "achthundert Metern",
}

PS_HEADER = r"""
Add-Type -AssemblyName System.Speech
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
"""

PS_LIST = PS_HEADER + r"$s.GetInstalledVoices() | % { $_.VoiceInfo.Name }"

PS_FMT = (r"$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo("
          r"16000,[System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,"
          r"[System.Speech.AudioFormat.AudioChannel]::Mono)")


def powershell(script: str) -> str:
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", script],
                       capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"powershell failed:\n{r.stderr}")
    return r.stdout


def list_voices():
    print(powershell(PS_LIST).strip())


def generate(voice: str | None):
    VOICE_DIR.mkdir(parents=True, exist_ok=True)
    pick = ""
    if voice:
        # match on a substring so "Hedda" finds "Microsoft Hedda Desktop"
        names = [n.strip() for n in powershell(PS_LIST).splitlines() if n.strip()]
        hit = next((n for n in names if voice.lower() in n.lower()), None)
        if not hit:
            sys.exit(f"no installed voice matches {voice!r}. --list-voices to see them.")
        pick = f'$s.SelectVoice("{hit}")'
        print(f"voice: {hit}")

    lines = [PS_HEADER, pick, PS_FMT]
    for stem, text in PHRASES.items():
        out = (VOICE_DIR / f"{stem}.wav").as_posix()
        safe = text.replace('"', '')
        lines.append(f'$s.SetOutputToWaveFile("{out}", $fmt); $s.Speak("{safe}"); $s.SetOutputToNull()')
    lines.append("$s.Dispose()")
    powershell("\n".join(lines))
    print(f"  {len(PHRASES)} voice clips -> {VOICE_DIR}")


def assemble_sd():
    (SD_AUDIO / "tones").mkdir(parents=True, exist_ok=True)
    (SD_AUDIO / "voice/de").mkdir(parents=True, exist_ok=True)
    for src, dst in ((TONES_DIR, SD_AUDIO / "tones"), (VOICE_DIR, SD_AUDIO / "voice/de")):
        for w in sorted(src.glob("*.wav")):
            shutil.copy2(w, dst / w.name)
    n = len(list(SD_AUDIO.rglob("*.wav")))
    print(f"  SD tree assembled: {n} clips in {SD_AUDIO}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--voice", help="SAPI voice name / substring (e.g. Hedda, Katja)")
    ap.add_argument("--list-voices", action="store_true")
    ap.add_argument("--upload", action="store_true", help="push tools/sd_root/audio to the device")
    ap.add_argument("--address", help="device BLE address for --upload")
    args = ap.parse_args()

    if args.list_voices:
        list_voices()
        return

    generate(args.voice)
    assemble_sd()

    if args.upload:
        sys.path.insert(0, str(ROOT / "tools"))
        import fake_app
        link = fake_app.Link()
        link.connect(address=args.address)
        try:
            link.upload_tree(str(SD_AUDIO), "audio")
        finally:
            link.disconnect()


if __name__ == "__main__":
    main()
