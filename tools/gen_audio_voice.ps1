# Generate German placeholder voice clips with Windows SAPI (Microsoft Hedda, de-DE)
# into tools/audio/voice/de/. Replace these with real recordings later.
#
#   powershell -ExecutionPolicy Bypass -File tools/gen_audio_voice.ps1

Add-Type -AssemblyName System.Speech
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
try { $synth.SelectVoice("Microsoft Hedda Desktop") } catch {
    Write-Warning "German voice 'Microsoft Hedda Desktop' not found - using default"
}
$synth.Rate = 1

$outDir = Join-Path $PSScriptRoot "audio\voice\de"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# 16 kHz, 16-bit, mono to match the tone assets and the codec config.
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(16000,
    [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,
    [System.Speech.AudioFormat.AudioChannel]::Mono)

$phrases = @{
    "achtung"            = "Achtung"
    "in"                 = "in"
    "blitzer_fest"       = "fester Blitzer"
    "blitzer_mobil"      = "mobiler Blitzer"
    "blitzer_anhaenger"  = "Blitzeranhänger"
    "blitzer_rotlicht"   = "Rotlichtblitzer"
    "blitzer_abstand"    = "Abstandsmessung"
    "gefahr_stau"        = "Stau"
    "gefahr_glaette"     = "Glätte"
    "gefahr_unfall"      = "Unfall"
    "gefahr_baustelle"   = "Baustelle"
    "gefahr_panne"       = "Panne"
    "gefahr_hindernis"   = "Hindernis auf der Fahrbahn"
    "d_100"              = "einhundert Metern"
    "d_200"              = "zweihundert Metern"
    "d_300"              = "dreihundert Metern"
    "d_500"              = "fünfhundert Metern"
    "d_800"              = "achthundert Metern"
}

foreach ($k in $phrases.Keys) {
    $path = Join-Path $outDir "$k.wav"
    $synth.SetOutputToWaveFile($path, $fmt)
    $synth.Speak($phrases[$k])
    $synth.SetOutputToNull()
    Write-Output "  $k.wav  <- `"$($phrases[$k])`""
}
$synth.Dispose()
Write-Output "done: $($phrases.Count) clips in $outDir"
