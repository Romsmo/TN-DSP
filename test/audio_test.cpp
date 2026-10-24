#include "doctest/doctest.h"

#include "core/audio/audio_service.h"
#include "core/audio/cue_composer.h"
#include "core/audio/wav.h"
#include "test/fakes.h"

using namespace core;
using namespace core::audio;

namespace {
AudioCue voice_cue(uint32_t seq, HazardType t, int dist) {
    AudioCue c;
    c.seq = seq;
    c.mode = AudioMode::Voice;
    c.event = AudioEvent::HazardApproach;
    c.hazardType = t;
    c.distanceM = dist;
    return c;
}
}  // namespace

TEST_CASE("WAV header parse: accepts 16-bit PCM, rejects the rest") {
    fakes::Storage sd;
    sd.put_wav("x", 100);
    WavInfo w;
    REQUIRE(parse_wav_header(sd.files["x"].data(), sd.files["x"].size(), w));
    CHECK(w.sample_rate == 16000);
    CHECK(w.channels == 1);
    CHECK(w.bits == 16);
    CHECK(w.data_bytes == 200);

    uint8_t junk[64] = {'R', 'I', 'F', 'F'};
    CHECK_FALSE(parse_wav_header(junk, sizeof(junk), w));
}

TEST_CASE("cue_composer: tones mode -> one event chime") {
    Clip pl[kMaxPlaylist];
    AudioCue c;
    c.mode = AudioMode::Tones;
    c.event = AudioEvent::HazardApproach;
    CHECK(compose(c, pl) == 1);
    CHECK(pl[0] == Clip::ToneWarn);

    c.event = AudioEvent::HazardImminent;
    CHECK(compose(c, pl) == 1);
    CHECK(pl[0] == Clip::ToneAlert);
}

TEST_CASE("cue_composer: voice mode -> Achtung + type + in + distance bucket") {
    Clip pl[kMaxPlaylist];
    int n = compose(voice_cue(1, HazardType::MobileSpeedCamera, 285), pl);
    REQUIRE(n == 4);
    CHECK(pl[0] == Clip::Achtung);
    CHECK(pl[1] == Clip::CamMobile);
    CHECK(pl[2] == Clip::In);
    CHECK(pl[3] == Clip::Dist200);

    CHECK(distance_clip(50) == Clip::Dist100);
    CHECK(distance_clip(300) == Clip::Dist300);
    CHECK(distance_clip(999) == Clip::Dist800);

    // no distance -> just Achtung + type
    AudioCue c = voice_cue(2, HazardType::Traffic, 0);
    c.distanceM.reset();
    n = compose(c, pl);
    CHECK(n == 2);
    CHECK(pl[1] == Clip::HazTraffic);

    // confirm prompt -> a soft chime regardless of mode
    AudioCue cp;
    cp.mode = AudioMode::Voice;
    cp.event = AudioEvent::ConfirmPrompt;
    CHECK(compose(cp, pl) == 1);
    CHECK(pl[0] == Clip::ToneConfirm);
}

TEST_CASE("AudioService: master gate, seq de-dup, amplifier lifecycle") {
    fakes::AudioDriver drv;
    fakes::Storage sd;
    for (int i = 0; i < kClipCount; ++i) sd.put_wav("/audio/" + std::string(clip_path(Clip(i))), 200);

    AudioService svc(&drv, &sd);

    Settings st;
    st.soundEnabled = false;
    svc.apply_settings(st);

    DisplayState s;
    s.audioCue = voice_cue(1, HazardType::Traffic, 300);
    svc.on_state(s);
    CHECK_FALSE(svc.playing());          // gated off (cue still counted as seen)

    st.soundEnabled = true;
    svc.apply_settings(st);
    s.audioCue = voice_cue(2, HazardType::Traffic, 300);   // fresh cue from the app
    svc.on_state(s);
    CHECK(svc.playing());
    CHECK(svc.last_seq() == 2);
    Clip pl[kMaxPlaylist];
    CHECK(svc.playlist(pl) == 4);

    svc.on_state(s);                     // same seq -> ignored (no restart)
    CHECK(svc.last_seq() == 2);

    // pump streams every clip, enables the amp, then stops + disables it
    CHECK(drv.amp);
    for (int i = 0; i < 500; ++i) svc.pump();
    CHECK_FALSE(svc.playing());
    CHECK_FALSE(drv.amp);
    CHECK(drv.written.size() > 0);
}

TEST_CASE("AudioService: falls back to a flash tone when the SD clip is missing") {
    fakes::AudioDriver drv;
    fakes::Storage sd;
    sd.is_mounted = false;               // no card

    AudioService svc(&drv, &sd);
    Settings st;
    st.soundEnabled = true;
    svc.apply_settings(st);

    DisplayState s;
    AudioCue c;
    c.seq = 7;
    c.mode = AudioMode::Tones;
    c.event = AudioEvent::HazardApproach;   // -> ToneWarn (has flash fallback)
    s.audioCue = c;
    svc.on_state(s);
    for (int i = 0; i < 500; ++i) svc.pump();
    CHECK(drv.written.size() > 0);          // played from flash
}

TEST_CASE("AudioService: a voice cue without the phrases on the card degrades to the chime") {
    fakes::AudioDriver drv;
    fakes::Storage sd;                   // mounted, but no voice clips (they are optional)
    AudioService svc(&drv, &sd);
    Settings st;
    st.soundEnabled = true;
    svc.apply_settings(st);

    DisplayState s;
    s.audioCue = voice_cue(3, HazardType::MobileSpeedCamera, 300);
    svc.on_state(s);
    REQUIRE(svc.playing());
    Clip pl[kMaxPlaylist];
    REQUIRE(svc.playlist(pl) == 1);
    CHECK(pl[0] == Clip::ToneWarn);      // not 4 silent phrases
    for (int i = 0; i < 500; ++i) svc.pump();
    CHECK(drv.written.size() > 0);       // the flash chime actually played
}

TEST_CASE("AudioService: one missing phrase is enough to fall back; a full set plays as voice") {
    fakes::AudioDriver drv;
    fakes::Storage sd;
    for (int i = 0; i < kClipCount; ++i) sd.put_wav("/audio/" + std::string(clip_path(Clip(i))), 200);
    sd.files.erase("/audio/" + std::string(clip_path(Clip::In)));

    AudioService svc(&drv, &sd);
    Settings st;
    st.soundEnabled = true;
    svc.apply_settings(st);

    DisplayState s;
    s.audioCue = voice_cue(1, HazardType::Traffic, 300);
    svc.on_state(s);
    Clip pl[kMaxPlaylist];
    CHECK(svc.playlist(pl) == 1);

    sd.put_wav("/audio/" + std::string(clip_path(Clip::In)), 200);
    s.audioCue = voice_cue(2, HazardType::Traffic, 300);
    svc.on_state(s);
    CHECK(svc.playlist(pl) == 4);
}

TEST_CASE("AudioService: a chime with no card and no flash copy still plays the warn chime") {
    fakes::AudioDriver drv;
    fakes::Storage sd;
    sd.is_mounted = false;
    AudioService svc(&drv, &sd);
    Settings st;
    st.soundEnabled = true;
    svc.apply_settings(st);

    DisplayState s;
    AudioCue c;
    c.seq = 9;
    c.mode = AudioMode::Tones;
    c.event = AudioEvent::HazardImminent;   // ToneAlert has no flash copy
    s.audioCue = c;
    svc.on_state(s);
    for (int i = 0; i < 500; ++i) svc.pump();
    CHECK(drv.written.size() > 0);
}
