#include "core/codec/protocol.h"

#include <variant>

#include "core/codec/cbor.h"

namespace core::proto {

using cbor::Reader;
using cbor::Type;
using cbor::Writer;

namespace {

// ── DisplayState keys ──────────────────────────────────────────────────────
enum SK {
    SK_VER = 0, SK_FLAGS1 = 1, SK_SPEED = 2, SK_SPEED_UNIT = 3, SK_HAZ_TYPE = 4,
    SK_HAZ_DIST = 5, SK_HAZ_SPEED = 6, SK_CONFIRM_TO = 7, SK_SEQ = 8, SK_EPOCH = 9,
    SK_PEERS = 10, SK_LEFT = 11, SK_RIGHT = 12, SK_CUE = 13,
};
enum SF {
    SF_INTERNET = 1 << 0, SF_ARROWS = 1 << 1, SF_HAZARD = 1 << 2,
    SF_CONFIRM = 1 << 3, SF_NIGHT = 1 << 4, SF_MPH = 1 << 5,
};
// AudioCue sub-map keys
enum CK { CK_SEQ = 0, CK_MODE = 1, CK_EVENT = 2, CK_HTYPE = 3, CK_DIST = 4, CK_SPEED = 5, CK_UNIT = 6 };

// ── DeviceEvent keys ───────────────────────────────────────────────────────
enum EK {
    EK_VER = 0, EK_TAG = 1, EK_HTYPE = 2, EK_SPEED = 3, EK_SEEN = 4, EK_ON = 5,
    EK_MODE = 6, EK_MPH = 7, EK_FILTER = 8, EK_UPTIME = 9,
};

// ── Settings keys ──────────────────────────────────────────────────────────
enum TK {
    TK_VER = 0, TK_SOUND = 1, TK_SOUNDMODE = 2, TK_THEME = 3, TK_MPH = 4,
    TK_FILTER = 5, TK_NIGHT_START = 6, TK_NIGHT_END = 7, TK_CACHE_MIN = 8,
    TK_VOLUME = 9,
};

// ── UiConfig keys ──────────────────────────────────────────────────────────
enum UK { UK_VER = 0, UK_BITS = 1 };

void put_signs(Writer& w, uint32_t key, const SignList& l) {
    w.key(key);
    w.array(static_cast<size_t>(l.size()));
    for (int i = 0; i < l.size(); ++i) w.uint(l[i]);
}

}  // namespace

// ───────────────────────────── encode ──────────────────────────────────────

size_t encode_state(const DisplayState& s, uint8_t* out, size_t cap) {
    Writer w(out, cap);

    uint32_t f1 = 0;
    if (s.internetConnected) f1 |= SF_INTERNET;
    if (s.directionArrows) f1 |= SF_ARROWS;
    if (s.hazardActive) f1 |= SF_HAZARD;
    if (s.confirmPromptActive) f1 |= SF_CONFIRM;
    if (s.nightMode) f1 |= SF_NIGHT;
    if (s.useMph) f1 |= SF_MPH;

    size_t n = 5;  // ver, flags1, unit, confirmTo, seq
    if (s.speedLimit) ++n;
    if (s.hazardActive && s.hazardType) ++n;
    if (s.hazardDistanceM) ++n;
    if (s.hazardSpeedLimit) ++n;
    if (s.epochS) ++n;
    if (s.networkPeers) ++n;
    if (!s.leftSigns.empty()) ++n;
    if (!s.rightSigns.empty()) ++n;
    if (s.audioCue) ++n;

    w.map(n);
    w.kv(SK_VER, kVersion);
    w.kv(SK_FLAGS1, f1);
    if (s.speedLimit) w.kv(SK_SPEED, static_cast<uint64_t>(*s.speedLimit));
    w.kv(SK_SPEED_UNIT, static_cast<uint64_t>(s.speedLimitUnit));
    if (s.hazardActive && s.hazardType) w.kv(SK_HAZ_TYPE, static_cast<uint64_t>(*s.hazardType));
    if (s.hazardDistanceM) w.kv(SK_HAZ_DIST, static_cast<uint64_t>(*s.hazardDistanceM));
    if (s.hazardSpeedLimit) w.kv(SK_HAZ_SPEED, static_cast<uint64_t>(*s.hazardSpeedLimit));
    w.kv(SK_CONFIRM_TO, static_cast<uint64_t>(s.confirmPromptTimeoutS));
    w.kv(SK_SEQ, s.stateSeq);
    if (s.epochS) w.kv(SK_EPOCH, s.epochS);
    if (s.networkPeers) w.kv(SK_PEERS, s.networkPeers);
    if (!s.leftSigns.empty()) put_signs(w, SK_LEFT, s.leftSigns);
    if (!s.rightSigns.empty()) put_signs(w, SK_RIGHT, s.rightSigns);
    if (s.audioCue) {
        const AudioCue& c = *s.audioCue;
        size_t cn = 4;  // seq, mode, event, unit
        if (c.hazardType) ++cn;
        if (c.distanceM) ++cn;
        if (c.speedValue) ++cn;
        w.key(SK_CUE);
        w.map(cn);
        w.kv(CK_SEQ, c.seq);
        w.kv(CK_MODE, static_cast<uint64_t>(c.mode));
        w.kv(CK_EVENT, static_cast<uint64_t>(c.event));
        if (c.hazardType) w.kv(CK_HTYPE, static_cast<uint64_t>(*c.hazardType));
        if (c.distanceM) w.kv(CK_DIST, static_cast<uint64_t>(*c.distanceM));
        if (c.speedValue) w.kv(CK_SPEED, static_cast<uint64_t>(*c.speedValue));
        w.kv(CK_UNIT, static_cast<uint64_t>(c.speedUnit));
    }
    return w.ok() ? w.size() : 0;
}

size_t encode_event(const DeviceEvent& e, uint8_t* out, size_t cap) {
    Writer w(out, cap);
    const auto tag = tag_of(e);

    auto begin = [&](size_t extra) {
        w.map(2 + extra);
        w.kv(EK_VER, kVersion);
        w.kv(EK_TAG, static_cast<uint64_t>(tag));
    };

    std::visit(
        [&](auto&& ev) {
            using T = std::decay_t<decltype(ev)>;
            if constexpr (std::is_same_v<T, ReportSubmitted>) {
                begin(ev.speedKmh ? 2u : 1u);
                w.kv(EK_HTYPE, static_cast<uint64_t>(ev.type));
                if (ev.speedKmh) w.kv(EK_SPEED, static_cast<uint64_t>(*ev.speedKmh));
            } else if constexpr (std::is_same_v<T, ConfirmResponse>) {
                begin(1);
                w.kv_bool(EK_SEEN, ev.seen);
            } else if constexpr (std::is_same_v<T, SettingsSoundToggle>) {
                begin(1);
                w.kv_bool(EK_ON, ev.on);
            } else if constexpr (std::is_same_v<T, SettingsThemeChanged>) {
                begin(1);
                w.kv(EK_MODE, static_cast<uint64_t>(ev.mode));
            } else if constexpr (std::is_same_v<T, SettingsUnitChanged>) {
                begin(1);
                w.kv_bool(EK_MPH, ev.useMph);
            } else if constexpr (std::is_same_v<T, SettingsFilterChanged>) {
                begin(1);
                w.kv(EK_FILTER, ev.enabledBits);
            } else if constexpr (std::is_same_v<T, Heartbeat>) {
                begin(1);
                w.kv(EK_UPTIME, ev.uptimeS);
            } else {
                begin(0);   // ReportPressed / SettingsPhoneConnect
            }
        },
        e);
    return w.ok() ? w.size() : 0;
}

size_t encode_settings(const Settings& s, uint8_t* out, size_t cap) {
    Writer w(out, cap);
    w.map(10);
    w.kv(TK_VER, kVersion);
    w.kv_bool(TK_SOUND, s.soundEnabled);
    w.kv(TK_SOUNDMODE, static_cast<uint64_t>(s.soundMode));
    w.kv(TK_VOLUME, s.soundVolume);
    w.kv(TK_THEME, static_cast<uint64_t>(s.theme));
    w.kv_bool(TK_MPH, s.useMph);
    w.kv(TK_FILTER, s.filter.bits());
    w.kv(TK_NIGHT_START, s.nightHours.startHour);
    w.kv(TK_NIGHT_END, s.nightHours.endHour);
    w.kv(TK_CACHE_MIN, s.cacheStaleMinutes);
    return w.ok() ? w.size() : 0;
}

size_t encode_device_info(const DeviceInfo& d, uint8_t* out, size_t cap) {
    Writer w(out, cap);
    w.map(5);
    w.kv(0, d.protoVersion);
    w.key(1);
    w.text(d.fwVersion);
    w.kv(2, d.caps);
    w.kv(3, d.width);
    w.kv(4, d.height);
    return w.ok() ? w.size() : 0;
}

size_t encode_ui_config(const UiConfig& c, uint8_t* out, size_t cap) {
    Writer w(out, cap);
    w.map(2);
    w.kv(UK_VER, kVersion);
    w.kv(UK_BITS, c.bits());
    return w.ok() ? w.size() : 0;
}

size_t encode_time(const TimeSync& t, uint8_t* out, size_t cap) {
    Writer w(out, cap);
    w.map(3);
    w.kv(0, kVersion);
    w.kv(1, t.epochS);
    w.key(2);
    w.integer(t.tzOffsetMin);
    return w.ok() ? w.size() : 0;
}

// ───────────────────────────── decode ──────────────────────────────────────

uint8_t peek_version(const uint8_t* in, size_t len) {
    Reader r(in, len);
    if (r.peek_type() != Type::Map) return 0;
    const size_t n = r.enter();
    if (n == 0) return 0;
    const uint64_t k = r.read_uint();
    if (k != 0 || !r.ok()) return 0;
    const uint64_t v = r.read_uint();
    return r.ok() ? static_cast<uint8_t>(v) : 0;
}

bool decode_state(const uint8_t* in, size_t len, DisplayState& out) {
    out = DisplayState{};
    Reader r(in, len);
    if (r.peek_type() != Type::Map) return false;
    const size_t n = r.enter();

    for (size_t i = 0; i < n && r.ok(); ++i) {
        const uint32_t key = static_cast<uint32_t>(r.read_uint());
        switch (key) {
            case SK_VER: r.read_uint(); break;
            case SK_FLAGS1: {
                const uint32_t f = static_cast<uint32_t>(r.read_uint());
                out.internetConnected = f & SF_INTERNET;
                out.directionArrows = f & SF_ARROWS;
                out.hazardActive = f & SF_HAZARD;
                out.confirmPromptActive = f & SF_CONFIRM;
                out.nightMode = f & SF_NIGHT;
                out.useMph = f & SF_MPH;
                break;
            }
            case SK_SPEED: out.speedLimit = static_cast<int>(r.read_uint()); break;
            case SK_SPEED_UNIT: out.speedLimitUnit = static_cast<SpeedUnit>(r.read_uint()); break;
            case SK_HAZ_TYPE: out.hazardType = static_cast<HazardType>(r.read_uint()); break;
            case SK_HAZ_DIST: out.hazardDistanceM = static_cast<int>(r.read_uint()); break;
            case SK_HAZ_SPEED: out.hazardSpeedLimit = static_cast<int>(r.read_uint()); break;
            case SK_CONFIRM_TO: out.confirmPromptTimeoutS = static_cast<int>(r.read_uint()); break;
            case SK_SEQ: out.stateSeq = static_cast<uint32_t>(r.read_uint()); break;
            case SK_EPOCH: out.epochS = static_cast<uint32_t>(r.read_uint()); break;
            case SK_PEERS: out.networkPeers = static_cast<uint32_t>(r.read_uint()); break;
            case SK_LEFT:
            case SK_RIGHT: {
                SignList& l = key == SK_LEFT ? out.leftSigns : out.rightSigns;
                const size_t cnt = r.enter();
                for (size_t j = 0; j < cnt && r.ok(); ++j) {
                    l.add(static_cast<uint16_t>(r.read_uint()));
                }
                break;
            }
            case SK_CUE: {
                AudioCue c;
                const size_t cn = r.enter();
                for (size_t j = 0; j < cn && r.ok(); ++j) {
                    const uint32_t ck = static_cast<uint32_t>(r.read_uint());
                    switch (ck) {
                        case CK_SEQ: c.seq = static_cast<uint32_t>(r.read_uint()); break;
                        case CK_MODE: c.mode = static_cast<AudioMode>(r.read_uint()); break;
                        case CK_EVENT: c.event = static_cast<AudioEvent>(r.read_uint()); break;
                        case CK_HTYPE: c.hazardType = static_cast<HazardType>(r.read_uint()); break;
                        case CK_DIST: c.distanceM = static_cast<int>(r.read_uint()); break;
                        case CK_SPEED: c.speedValue = static_cast<int>(r.read_uint()); break;
                        case CK_UNIT: c.speedUnit = static_cast<SpeedUnit>(r.read_uint()); break;
                        default: r.skip(); break;
                    }
                }
                out.audioCue = c;
                break;
            }
            default: r.skip(); break;
        }
    }
    return r.ok();
}

bool decode_event(const uint8_t* in, size_t len, DeviceEvent& out) {
    Reader r(in, len);
    if (r.peek_type() != Type::Map) return false;
    const size_t n = r.enter();

    uint64_t tag = 0;
    HazardType htype{};
    std::optional<int> speed;
    bool seen = false, on = false, mph = false;
    uint64_t mode = 0, filter = 0xFFFF, uptime = 0;

    for (size_t i = 0; i < n && r.ok(); ++i) {
        const uint32_t key = static_cast<uint32_t>(r.read_uint());
        switch (key) {
            case EK_VER: r.read_uint(); break;
            case EK_TAG: tag = r.read_uint(); break;
            case EK_HTYPE: htype = static_cast<HazardType>(r.read_uint()); break;
            case EK_SPEED: speed = static_cast<int>(r.read_uint()); break;
            case EK_SEEN: seen = r.read_bool(); break;
            case EK_ON: on = r.read_bool(); break;
            case EK_MODE: mode = r.read_uint(); break;
            case EK_MPH: mph = r.read_bool(); break;
            case EK_FILTER: filter = r.read_uint(); break;
            case EK_UPTIME: uptime = r.read_uint(); break;
            default: r.skip(); break;
        }
    }
    if (!r.ok()) return false;

    switch (static_cast<EventTag>(tag)) {
        case EventTag::ReportPressed: out = ReportPressed{}; break;
        case EventTag::ReportSubmitted: out = ReportSubmitted{htype, speed}; break;
        case EventTag::ConfirmResponse: out = ConfirmResponse{seen}; break;
        case EventTag::SettingsPhoneConnect: out = SettingsPhoneConnect{}; break;
        case EventTag::SettingsSoundToggle: out = SettingsSoundToggle{on}; break;
        case EventTag::SettingsThemeChanged:
            out = SettingsThemeChanged{static_cast<ThemeMode>(mode)};
            break;
        case EventTag::SettingsUnitChanged: out = SettingsUnitChanged{mph}; break;
        case EventTag::SettingsFilterChanged:
            out = SettingsFilterChanged{static_cast<uint16_t>(filter)};
            break;
        case EventTag::Heartbeat: out = Heartbeat{static_cast<uint32_t>(uptime)}; break;
        default: return false;
    }
    return true;
}

bool decode_settings(const uint8_t* in, size_t len, Settings& out) {
    out = Settings{};
    Reader r(in, len);
    if (r.peek_type() != Type::Map) return false;
    const size_t n = r.enter();

    for (size_t i = 0; i < n && r.ok(); ++i) {
        const uint32_t key = static_cast<uint32_t>(r.read_uint());
        switch (key) {
            case TK_VER: r.read_uint(); break;
            case TK_SOUND: out.soundEnabled = r.read_bool(); break;
            case TK_SOUNDMODE: out.soundMode = static_cast<SoundMode>(r.read_uint()); break;
            case TK_VOLUME: {
                const uint64_t v = r.read_uint();
                out.soundVolume = static_cast<uint8_t>(v > 100 ? 100 : v);
                break;
            }
            case TK_THEME: out.theme = static_cast<ThemeMode>(r.read_uint()); break;
            case TK_MPH: out.useMph = r.read_bool(); break;
            case TK_FILTER: out.filter.set_bits(static_cast<uint16_t>(r.read_uint())); break;
            case TK_NIGHT_START: out.nightHours.startHour = static_cast<uint8_t>(r.read_uint()); break;
            case TK_NIGHT_END: out.nightHours.endHour = static_cast<uint8_t>(r.read_uint()); break;
            case TK_CACHE_MIN: out.cacheStaleMinutes = static_cast<uint16_t>(r.read_uint()); break;
            default: r.skip(); break;
        }
    }
    return r.ok();
}

bool decode_ui_config(const uint8_t* in, size_t len, UiConfig& out) {
    out = UiConfig{};
    Reader r(in, len);
    if (r.peek_type() != Type::Map) return false;
    const size_t n = r.enter();
    for (size_t i = 0; i < n && r.ok(); ++i) {
        const uint32_t key = static_cast<uint32_t>(r.read_uint());
        switch (key) {
            case UK_VER: r.read_uint(); break;
            case UK_BITS: out.set_bits(static_cast<uint32_t>(r.read_uint())); break;
            default: r.skip(); break;
        }
    }
    return r.ok();
}

bool decode_time(const uint8_t* in, size_t len, TimeSync& out) {
    out = TimeSync{};
    Reader r(in, len);
    if (r.peek_type() != Type::Map) return false;
    const size_t n = r.enter();
    for (size_t i = 0; i < n && r.ok(); ++i) {
        const uint32_t key = static_cast<uint32_t>(r.read_uint());
        switch (key) {
            case 0: r.read_uint(); break;
            case 1: out.epochS = static_cast<uint32_t>(r.read_uint()); break;
            case 2: out.tzOffsetMin = static_cast<int16_t>(r.read_int()); break;
            default: r.skip(); break;
        }
    }
    return r.ok();
}

}  // namespace core::proto
