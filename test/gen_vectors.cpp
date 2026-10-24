// test/gen_vectors.cpp — writes the golden test vectors of the BLE contract
// (docs/contract/vectors/*.json) from the REAL firmware codec.
//
// Every vector is: a hex payload + what core/codec/protocol.cpp decodes from it.
//   * "encoded" vectors: the codec ENCODED a model value (canonical byte form).
//   * "input" vectors: bytes written here by hand (old apps, unknown keys,
//     truncation); the expected result is whatever the firmware decoder returns.
// An app implementation must decode every vector to exactly "decoded", and
// (for "encoded" vectors of app->device characteristics) should produce the same bytes.
//
//   bdsp_gen_vectors <out-dir>        regenerate;  CI re-runs it and fails on a diff.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

#include "core/audio/clips.h"
#include "core/audio/cue_composer.h"
#include "core/catalog/category_registry.h"
#include "core/codec/protocol.h"
#include "core/model/ui_config.h"
#if defined(BDSP_HAVE_UI)
#include "ui/i18n/category_label.h"
#endif

using namespace core;
using namespace core::proto;

namespace {

// ── tiny JSON writer ────────────────────────────────────────────────────────
std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') o += '\\';
        o += c;
    }
    return o;
}
std::string q(const std::string& s) { return "\"" + esc(s) + "\""; }
std::string num(long long v) { return std::to_string(v); }
std::string bl(bool b) { return b ? "true" : "false"; }
template <typename T>
std::string opt(const std::optional<T>& v) {
    return v ? num(static_cast<long long>(*v)) : "null";
}
std::string arr(const SignList& l) {
    std::string o = "[";
    for (int i = 0; i < l.size(); ++i) o += (i ? "," : "") + num(l[i]);
    return o + "]";
}
std::string hex(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string o;
    for (size_t i = 0; i < n; ++i) {
        o += d[p[i] >> 4];
        o += d[p[i] & 15];
    }
    return o;
}
std::vector<uint8_t> unhex(const std::string& h) {
    std::vector<uint8_t> v;
    for (size_t i = 0; i + 1 < h.size(); i += 2) {
        v.push_back(static_cast<uint8_t>(std::strtol(h.substr(i, 2).c_str(), nullptr, 16)));
    }
    return v;
}

// ── model -> JSON ───────────────────────────────────────────────────────────
std::string j(const AudioCue& c) {
    std::optional<int> ht;
    if (c.hazardType) ht = static_cast<int>(*c.hazardType);
    return "{\"seq\":" + num(c.seq) + ",\"mode\":" + num(static_cast<int>(c.mode)) +
           ",\"event\":" + num(static_cast<int>(c.event)) + ",\"hazardType\":" + opt(ht) +
           ",\"distanceM\":" + opt(c.distanceM) + ",\"speedValue\":" + opt(c.speedValue) +
           ",\"speedUnit\":" + num(static_cast<int>(c.speedUnit)) + "}";
}
std::string j(const DisplayState& s) {
    std::optional<int> ht;
    if (s.hazardType) ht = static_cast<int>(*s.hazardType);
    return "{\"internetConnected\":" + bl(s.internetConnected) +
           ",\"networkPeers\":" + num(s.networkPeers) + ",\"speedLimit\":" + opt(s.speedLimit) +
           ",\"speedLimitUnit\":" + num(static_cast<int>(s.speedLimitUnit)) +
           ",\"directionArrows\":" + bl(s.directionArrows) + ",\"leftSigns\":" + arr(s.leftSigns) +
           ",\"rightSigns\":" + arr(s.rightSigns) + ",\"hazardActive\":" + bl(s.hazardActive) +
           ",\"hazardType\":" + opt(ht) + ",\"hazardDistanceM\":" + opt(s.hazardDistanceM) +
           ",\"hazardSpeedLimit\":" + opt(s.hazardSpeedLimit) +
           ",\"confirmPromptActive\":" + bl(s.confirmPromptActive) +
           ",\"confirmPromptTimeoutS\":" + num(s.confirmPromptTimeoutS) +
           ",\"nightMode\":" + bl(s.nightMode) + ",\"useMph\":" + bl(s.useMph) +
           ",\"stateSeq\":" + num(s.stateSeq) + ",\"epochS\":" + num(s.epochS) +
           ",\"audioCue\":" + (s.audioCue ? j(*s.audioCue) : "null") + "}";
}
std::string j(const Settings& s) {
    return "{\"soundEnabled\":" + bl(s.soundEnabled) +
           ",\"soundMode\":" + num(static_cast<int>(s.soundMode)) +
           ",\"soundVolume\":" + num(s.soundVolume) + ",\"theme\":" + num(static_cast<int>(s.theme)) +
           ",\"useMph\":" + bl(s.useMph) + ",\"filterBits\":" + num(s.filter.bits()) +
           ",\"nightStartHour\":" + num(s.nightHours.startHour) +
           ",\"nightEndHour\":" + num(s.nightHours.endHour) +
           ",\"cacheStaleMinutes\":" + num(s.cacheStaleMinutes) + "}";
}
std::string j(const UiConfig& c) { return "{\"bits\":" + num(c.bits()) + "}"; }
std::string j(const TimeSync& t) {
    return "{\"epochS\":" + num(t.epochS) + ",\"tzOffsetMin\":" + num(t.tzOffsetMin) + "}";
}
std::string j(const DeviceInfo& d) {
    return "{\"protoVersion\":" + num(d.protoVersion) + ",\"fwVersion\":" + q(d.fwVersion) +
           ",\"caps\":" + num(d.caps) + ",\"width\":" + num(d.width) +
           ",\"height\":" + num(d.height) + "}";
}
std::string j(const DeviceEvent& e) {
    const int tag = static_cast<int>(tag_of(e));
    std::string o = "{\"tag\":" + num(tag);
    if (auto* r = std::get_if<ReportSubmitted>(&e)) {
        o += ",\"hazardType\":" + num(static_cast<int>(r->type)) + ",\"speedKmh\":" + opt(r->speedKmh);
    } else if (auto* c = std::get_if<ConfirmResponse>(&e)) {
        o += ",\"seen\":" + bl(c->seen);
    } else if (auto* so = std::get_if<SettingsSoundToggle>(&e)) {
        o += ",\"on\":" + bl(so->on);
    } else if (auto* th = std::get_if<SettingsThemeChanged>(&e)) {
        o += ",\"mode\":" + num(static_cast<int>(th->mode));
    } else if (auto* u = std::get_if<SettingsUnitChanged>(&e)) {
        o += ",\"useMph\":" + bl(u->useMph);
    } else if (auto* f = std::get_if<SettingsFilterChanged>(&e)) {
        o += ",\"filterBits\":" + num(f->enabledBits);
    } else if (auto* h = std::get_if<Heartbeat>(&e)) {
        o += ",\"uptimeS\":" + num(h->uptimeS);
    }
    return o + "}";
}

// ── vector bookkeeping ──────────────────────────────────────────────────────
struct Vector {
    std::string name, kind, note, hexs, decoded;   // decoded == "" -> decode must FAIL
    bool        canonical;                          // true: byte-exact encoder output
};
using Vectors = std::vector<Vector>;

template <typename Model, typename EncodeFn, typename DecodeFn>
void add_encoded(Vectors& out, const char* name, const char* note, const Model& m, EncodeFn enc,
                 DecodeFn dec) {
    uint8_t buf[kMaxPayload];
    const size_t n = enc(m, buf, sizeof(buf));
    Model back{};
    const bool ok = dec(buf, n, back);
    if (!ok || n == 0) {
        std::fprintf(stderr, "vector %s: codec round trip failed\n", name);
        std::exit(1);
    }
    out.push_back({name, "encoded", note, hex(buf, n), j(back), true});
}

template <typename Model, typename DecodeFn>
void add_input(Vectors& out, const char* name, const char* note, const std::string& hexs,
               DecodeFn dec) {
    const auto bytes = unhex(hexs);
    Model m{};
    const bool ok = dec(bytes.data(), bytes.size(), m);
    out.push_back({name, "input", note, hexs, ok ? j(m) : "", false});
}

void write(const std::string& dir, const char* file, const char* slot, const char* dir_txt,
           const char* what, const Vectors& v) {
    std::string o = "{\n  \"characteristic\": " + q(slot) + ",\n  \"direction\": " + q(dir_txt) +
                    ",\n  \"what\": " + q(what) + ",\n  \"protocolVersion\": " + num(kVersion) +
                    ",\n  \"generatedBy\": \"test/gen_vectors.cpp (core/codec/protocol.cpp)\"" +
                    ",\n  \"vectors\": [\n";
    for (size_t i = 0; i < v.size(); ++i) {
        const Vector& x = v[i];
        o += "    {\n      \"name\": " + q(x.name) + ",\n      \"kind\": " + q(x.kind) +
             ",\n      \"canonical\": " + bl(x.canonical) + ",\n      \"note\": " + q(x.note) +
             ",\n      \"hex\": " + q(x.hexs) + ",\n      \"decodeOk\": " + bl(!x.decoded.empty()) +
             ",\n      \"decoded\": " + (x.decoded.empty() ? "null" : x.decoded) + "\n    }" +
             (i + 1 < v.size() ? "," : "") + "\n";
    }
    o += "  ]\n}\n";
    std::ofstream f(dir + "/" + file, std::ios::binary);
    f << o;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: bdsp_gen_vectors <out-dir>\n");
        return 2;
    }
    const std::string dir = argv[1];

    // ── State (0001, app -> device) ────────────────────────────────────────
    {
        Vectors v;
        auto enc = [](const DisplayState& s, uint8_t* b, size_t c) { return encode_state(s, b, c); };
        auto dec = [](const uint8_t* b, size_t n, DisplayState& s) { return decode_state(b, n, s); };

        DisplayState home;
        home.internetConnected = true;
        home.directionArrows = true;
        home.speedLimit = 60;
        home.networkPeers = 480000;
        home.leftSigns.add(27600);
        home.rightSigns.add(14210);
        home.rightSigns.add(11004);
        home.epochS = 1'800'000'000;
        home.stateSeq = 7;
        add_encoded(v, "home-clear-road", "60 km/h, free-road picture, one prohibition left, two danger signs right", home, enc, dec);

        DisplayState bare;
        bare.stateSeq = 1;
        add_encoded(v, "minimal", "nothing but the sequence number: no sign, offline, no peers", bare, enc, dec);

        DisplayState haz = home;
        haz.hazardActive = true;
        haz.hazardType = HazardType::MobileSpeedCamera;
        haz.hazardDistanceM = 300;
        haz.hazardSpeedLimit = 50;
        haz.stateSeq = 8;
        AudioCue cue;
        cue.seq = 1;
        cue.mode = AudioMode::Voice;
        cue.event = AudioEvent::HazardApproach;
        cue.hazardType = HazardType::MobileSpeedCamera;
        cue.distanceM = 300;
        cue.speedValue = 50;
        haz.audioCue = cue;
        add_encoded(v, "hazard-mobile-camera-with-voice-cue", "overlay at 300 m, enforced limit 50, voice announcement", haz, enc, dec);

        DisplayState conf;
        conf.internetConnected = true;
        conf.speedLimit = 30;
        conf.speedLimitUnit = SpeedUnit::Mph;
        conf.useMph = true;
        conf.nightMode = true;
        conf.confirmPromptActive = true;
        conf.confirmPromptTimeoutS = 12;
        conf.stateSeq = 9;
        add_encoded(v, "confirm-prompt-night-mph", "Gesehen? prompt, 12 s, night dim, a 30 mph SIGN value", conf, enc, dec);

        DisplayState unk = bare;
        unk.stateSeq = 10;
        unk.hazardActive = true;
        unk.hazardType = static_cast<HazardType>(11);
        unk.hazardDistanceM = 400;
        add_encoded(v, "hazard-unknown-type-11", "type 11 is not defined in v1: the device must render a generic icon, not fail", unk, enc, dec);

        DisplayState four = bare;
        four.stateSeq = 11;
        for (uint16_t c : std::initializer_list<uint16_t>{10100, 10151, 11004, 12300}) four.rightSigns.add(c);
        for (uint16_t c : std::initializer_list<uint16_t>{27600, 28300, 28600, 25000}) four.leftSigns.add(c);
        add_encoded(v, "four-signs-per-column", "the wire carries up to 4 per column; the display shows the newest 3", four, enc, dec);

        DisplayState tone = bare;
        tone.stateSeq = 12;
        AudioCue t;
        t.seq = 2;
        t.mode = AudioMode::Tones;
        t.event = AudioEvent::HazardImminent;
        tone.audioCue = t;
        add_encoded(v, "audio-cue-tone", "chime only, no hazard fields", tone, enc, dec);

        // hand-written inputs
        add_input<DisplayState>(v, "forward-compat-unknown-keys", "keys 99 (uint) and 100 (nested map) are unknown and must be skipped",
                                "a400010218321863071864a1010a", dec);
        add_input<DisplayState>(v, "malformed-truncated", "a map that announces more pairs than the payload holds: decode must fail",
                                "a500010102", dec);
        add_input<DisplayState>(v, "malformed-not-a-map", "a bare integer: decode must fail", "01", dec);
        write(dir, "state.json", "0001", "app -> device", "DisplayState", v);
    }

    // ── Event (0002, device -> app) ────────────────────────────────────────
    {
        Vectors v;
        auto enc = [](const DeviceEvent& e, uint8_t* b, size_t c) { return encode_event(e, b, c); };
        auto dec = [](const uint8_t* b, size_t n, DeviceEvent& e) { return decode_event(b, n, e); };
        add_encoded(v, "report-pressed", "the + button was tapped", DeviceEvent{ReportPressed{}}, enc, dec);
        add_encoded(v, "report-submitted-with-speed", "Melden: mobile camera, speed in km/h", DeviceEvent{ReportSubmitted{HazardType::MobileSpeedCamera, 50}}, enc, dec);
        add_encoded(v, "report-submitted-no-speed", "Melden: accident (no speed field)", DeviceEvent{ReportSubmitted{HazardType::Accident, std::nullopt}}, enc, dec);
        add_encoded(v, "confirm-seen", "Gesehen? answered Ja", DeviceEvent{ConfirmResponse{true}}, enc, dec);
        add_encoded(v, "confirm-not-seen", "Gesehen? answered Nein", DeviceEvent{ConfirmResponse{false}}, enc, dec);
        add_encoded(v, "phone-connect", "legacy: the pairing button (no longer sent by the UI)", DeviceEvent{SettingsPhoneConnect{}}, enc, dec);
        add_encoded(v, "sound-on", "Settings: sound switched on", DeviceEvent{SettingsSoundToggle{true}}, enc, dec);
        add_encoded(v, "theme-changed", "Settings: theme (the device has one dark theme; kept for the wire)", DeviceEvent{SettingsThemeChanged{ThemeMode::Night}}, enc, dec);
        add_encoded(v, "units-mph", "Settings: units switched to mph", DeviceEvent{SettingsUnitChanged{true}}, enc, dec);
        add_encoded(v, "filter-changed", "Settings: hazard filter bits changed on the device", DeviceEvent{SettingsFilterChanged{0x07FB}}, enc, dec);
        add_encoded(v, "heartbeat", "every ~10 s while connected; uptime in seconds", DeviceEvent{Heartbeat{12345}}, enc, dec);
        write(dir, "event.json", "0002", "device -> app", "DeviceEvent", v);
    }

    // ── Settings (0004, both ways) ─────────────────────────────────────────
    {
        Vectors v;
        auto enc = [](const Settings& s, uint8_t* b, size_t c) { return encode_settings(s, b, c); };
        auto dec = [](const uint8_t* b, size_t n, Settings& s) { return decode_settings(b, n, s); };
        add_encoded(v, "defaults", "a factory-fresh device", Settings{}, enc, dec);
        Settings s;
        s.soundEnabled = true;
        s.soundMode = SoundMode::Voice;
        s.soundVolume = 40;
        s.useMph = true;
        s.filter.set(HazardType::Traffic, false);
        s.nightHours = {21, 6};
        s.cacheStaleMinutes = 15;
        add_encoded(v, "custom", "sound on, voice, volume 40, mph, Stau filtered out, night 21-06, cache 15 min", s, enc, dec);
        Settings loud = s;
        loud.soundVolume = 100;
        add_encoded(v, "volume-100", "upper bound of the volume range", loud, enc, dec);
        add_input<Settings>(v, "v1-writer-without-volume", "an older app that never sends key 9: volume keeps the default 70",
                            "a9000101f50201030004f50519fffb06150706080f", dec);
        add_input<Settings>(v, "volume-out-of-range-clamps", "key 9 = 200 is clamped to 100",
                            "a200010918c8", dec);
        write(dir, "settings.json", "0004", "both", "Settings", v);
    }

    // ── Time (0005, app -> device) ─────────────────────────────────────────
    {
        Vectors v;
        auto enc = [](const TimeSync& t, uint8_t* b, size_t c) { return encode_time(t, b, c); };
        auto dec = [](const uint8_t* b, size_t n, TimeSync& t) { return decode_time(b, n, t); };
        add_encoded(v, "utc-plus-2", "epoch seconds + timezone offset in minutes", TimeSync{1'800'000'000u, 120}, enc, dec);
        add_encoded(v, "utc-minus-3-30", "negative offsets are CBOR negative integers", TimeSync{1'800'000'000u, -210}, enc, dec);
        write(dir, "time.json", "0005", "app -> device", "TimeSync", v);
    }

    // ── UiConfig (0006, both ways) ─────────────────────────────────────────
    {
        Vectors v;
        auto enc = [](const UiConfig& c, uint8_t* b, size_t n) { return encode_ui_config(c, b, n); };
        auto dec = [](const uint8_t* b, size_t n, UiConfig& c) { return decode_ui_config(b, n, c); };
        add_encoded(v, "all-on", "every feature enabled (factory state)", UiConfig{}, enc, dec);
        UiConfig c;
        c.meldenPage = false;
        c.peerCount = false;
        c.bootAnimation = false;
        add_encoded(v, "melden-peers-boot-off", "Melden page, peer count and boot animation stripped", c, enc, dec);
        write(dir, "ui-config.json", "0006", "both", "UiConfig", v);
    }

    // ── DeviceInfo (0003, device -> app; read) ─────────────────────────────
    {
        Vectors v;
        DeviceInfo d;
        d.fwVersion = "0.1.0-dev";
        d.caps = 61;
        uint8_t buf[kMaxPayload];
        const size_t n = encode_device_info(d, buf, sizeof(buf));
        v.push_back({"typical", "encoded",
                     "caps 61 = audio + rtc + sd + speedcam + ota (bits 0,2,3,4,5); see ble-protocol.md",
                     hex(buf, n), j(d), true});
        write(dir, "device-info.json", "0003", "device -> app", "DeviceInfo", v);
    }

    // ── constants ──────────────────────────────────────────────────────────
    {
        std::string o = std::string("{\n  \"protocolVersion\": ") + num(kVersion) +
                        ",\n  \"maxPayloadBytes\": " + num(static_cast<long long>(kMaxPayload)) +
                        ",\n  \"uiConfigBitCount\": " + num(static_cast<long long>(UiConfig::kBitCount)) +
                        ",\n  \"hazardTypeMaxKnown\": " + num(kHazardTypeMaxKnown) +
                        ",\n  \"signListMax\": " + num(SignList::kMax) + "\n}\n";
        std::ofstream f(dir + "/constants.json", std::ios::binary);
        f << o;
    }

    // ── categories + audio vocabulary (from the firmware's own tables) ──────
#if defined(BDSP_HAVE_UI)
    {
        auto name_of = [](HazardType t) -> const char* {
            switch (t) {
                case HazardType::FixedSpeedCamera:  return "FixedSpeedCamera";
                case HazardType::MobileSpeedCamera: return "MobileSpeedCamera";
                case HazardType::TrailerCamera:     return "TrailerCamera";
                case HazardType::RedLightCamera:    return "RedLightCamera";
                case HazardType::DistanceControl:   return "DistanceControl";
                case HazardType::Traffic:           return "Traffic";
                case HazardType::Ice:               return "Ice";
                case HazardType::Accident:          return "Accident";
                case HazardType::Construction:      return "Construction";
                case HazardType::Breakdown:         return "Breakdown";
                case HazardType::Obstacle:          return "Obstacle";
            }
            return "";
        };
        std::string o = "{\n  \"protocolVersion\": " + num(kVersion) +
            ",\n  \"note\": \"HazardType values are frozen for protocol v1. filterBits (Settings key 5) "
            "uses the same numbers as bit indices. The speed-camera group (ids 0-4) exists only in "
            "firmware built with the speedcam capability (DeviceInfo caps bit 4).\"" +
            ",\n  \"unknownTypeBehaviour\": \"a hazardType above 10 renders a generic hazard icon; it is never "
            "rejected and is never filtered out\"" +
            ",\n  \"categories\": [\n";
        const Category* first = CategoryRegistry::begin();
        for (const Category* c = first; c != CategoryRegistry::end(); ++c) {
            AudioCue cue;
            cue.mode = AudioMode::Voice;
            cue.event = AudioEvent::HazardApproach;
            cue.hazardType = c->type;
            audio::Clip pl[audio::kMaxPlaylist];
            const int n = audio::compose(cue, pl);
            const std::string voice = n >= 2 ? audio::clip_path(pl[1]) : "";
            o += std::string(c == first ? "" : ",\n") + "    {\"id\": " + num(static_cast<int>(c->type)) +
                 ", \"key\": " + q(name_of(c->type)) +
                 ", \"labelDe\": " + q(ui::i18n::category_label(c->type)) +
                 ", \"group\": " + q(c->group == HazardGroup::SpeedCamera ? "speedCamera" : "generalHazard") +
                 ", \"needsSpeed\": " + bl(c->needs_speed) +
                 ", \"filterBit\": " + num(static_cast<int>(c->type)) +
                 ", \"voicePhrase\": " + q(voice) + "}";
        }
        o += "\n  ],\n  \"audio\": {\n    \"events\": [\n";
        const AudioEvent evs[] = {AudioEvent::HazardApproach, AudioEvent::HazardImminent,
                                  AudioEvent::ConfirmPrompt, AudioEvent::Cleared, AudioEvent::Info};
        const char* ev_names[] = {"HazardApproach", "HazardImminent", "ConfirmPrompt", "Cleared", "Info"};
        for (int i = 0; i < 5; ++i) {
            AudioCue cue;
            cue.mode = AudioMode::Tones;
            cue.event = evs[i];
            audio::Clip pl[audio::kMaxPlaylist];
            audio::compose(cue, pl);
            o += std::string(i ? ",\n" : "") + "      {\"id\": " + num(i) + ", \"name\": " + q(ev_names[i]) +
                 ", \"toneClip\": " + q(audio::clip_path(pl[0])) + "}";
        }
        o += "\n    ],\n    \"voicePhrases\": {\"achtung\": " + q(audio::clip_path(audio::Clip::Achtung)) +
             ", \"in\": " + q(audio::clip_path(audio::Clip::In)) + "},\n    \"distanceBuckets\": [\n";
        const int metres[] = {100, 200, 300, 500, 800};
        for (int i = 0; i < 5; ++i) {
            o += std::string(i ? ",\n" : "") + "      {\"fromMetres\": " + num(metres[i]) +
                 ", \"clip\": " + q(audio::clip_path(audio::distance_clip(metres[i]))) + "}";
        }
        o += "\n    ],\n    \"voiceAnnouncementRule\": \"Voice + HazardApproach/HazardImminent: achtung, <type phrase>, "
             "then (only if distanceM >= 50) in + the bucket clip rounded DOWN; ConfirmPrompt and Cleared play "
             "their tone clip in either mode. If any phrase clip is missing on the card the device plays the "
             "event chime instead.\",\n    \"format\": \"WAV, 16-bit PCM, mono, 16 kHz\"\n  }\n}\n";
        std::ofstream f(dir + "/categories.json", std::ios::binary);
        f << o;
    }
#endif
    std::printf("vectors written to %s\n", dir.c_str());
    return 0;
}
