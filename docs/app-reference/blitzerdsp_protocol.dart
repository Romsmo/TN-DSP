// blitzerdsp_protocol.dart — REFERENCE SNIPPET for the future Flutter app.
// Not a runnable app. Mirrors core/codec/protocol.cpp and docs/ble-protocol.md.
//
// deps:  cbor: ^6   flutter_blue_plus: ^1  (or your BLE lib of choice)

import 'package:cbor/cbor.dart';

const int kProtocolVersion = 1;

// PROVISIONAL UUID base 079aXXXX-d673-469c-98a0-12c0c5eb43b3
String bleUuid(int slot) =>
    '079a${slot.toRadixString(16).padLeft(4, '0')}-d673-469c-98a0-12c0c5eb43b3';
final serviceUuid = bleUuid(0x0000);
final stateUuid = bleUuid(0x0001);
final eventUuid = bleUuid(0x0002);
final infoUuid = bleUuid(0x0003);
final settingsUuid = bleUuid(0x0004);
final timeUuid = bleUuid(0x0005);

enum SpeedUnit { kmh, mph }

// Frozen for protocol v1 — never reorder.
enum HazardType {
  fixedSpeedCamera, mobileSpeedCamera, trailerCamera, redLightCamera, distanceControl,
  traffic, ice, accident, construction, breakdown, obstacle,
}

enum AudioMode { tones, voice }
enum AudioEvent { hazardApproach, hazardImminent, confirmPrompt, cleared, info }

class AudioCue {
  int seq = 0;
  AudioMode mode = AudioMode.tones;
  AudioEvent event = AudioEvent.info;
  HazardType? hazardType;
  int? distanceM;
  int? speedValue;
  SpeedUnit speedUnit = SpeedUnit.kmh;

  Map<int, Object?> toMap() => {
        0: seq,
        1: mode.index,
        2: event.index,
        if (hazardType != null) 3: hazardType!.index,
        if (distanceM != null) 4: distanceM,
        if (speedValue != null) 5: speedValue,
        6: speedUnit.index,
      };
}

/// App → device. Everything the board should show. The board renders these
/// fields and derives nothing else.
class DisplayState {
  bool internetConnected = false;
  int networkPeers = 0;
  int? speedLimit; // a SIGN value, shown unchanged
  SpeedUnit speedLimitUnit = SpeedUnit.kmh;
  bool directionArrows = false;
  List<int> leftSigns = []; // VzNr codes major*100+minor, prohibitions
  List<int> rightSigns = []; // VzNr codes, danger signs
  bool hazardActive = false;
  HazardType? hazardType;
  int? hazardDistanceM;
  int? hazardSpeedLimit;
  bool confirmPromptActive = false;
  int confirmPromptTimeoutS = 15;
  bool nightMode = false;
  bool useMph = false;
  int stateSeq = 0;
  int epochS = 0;
  AudioCue? audioCue;

  static const _fInternet = 1, _fArrows = 2, _fHazard = 4, _fConfirm = 8, _fNight = 16, _fMph = 32;

  List<int> encode() {
    var flags = 0;
    if (internetConnected) flags |= _fInternet;
    if (directionArrows) flags |= _fArrows;
    if (hazardActive) flags |= _fHazard;
    if (confirmPromptActive) flags |= _fConfirm;
    if (nightMode) flags |= _fNight;
    if (useMph) flags |= _fMph;

    final m = <int, Object?>{
      0: kProtocolVersion,
      1: flags,
      3: speedLimitUnit.index,
      7: confirmPromptTimeoutS,
      8: stateSeq,
      if (speedLimit != null) 2: speedLimit,
      if (hazardActive && hazardType != null) 4: hazardType!.index,
      if (hazardDistanceM != null) 5: hazardDistanceM,
      if (hazardSpeedLimit != null) 6: hazardSpeedLimit,
      if (epochS != 0) 9: epochS,
      if (networkPeers != 0) 10: networkPeers,
      if (leftSigns.isNotEmpty) 11: leftSigns,
      if (rightSigns.isNotEmpty) 12: rightSigns,
      if (audioCue != null) 13: audioCue!.toMap(),
    };
    return cbor.encode(CborValue(m));
  }
}

/// Device → app. Raw interaction facts only.
class DeviceEvent {
  final int tag;
  final Map<int, Object?> extra;
  DeviceEvent(this.tag, this.extra);

  static const reportPressed = 0,
      reportSubmitted = 1,
      confirmResponse = 2,
      settingsPhoneConnect = 3,
      settingsSoundToggle = 4,
      settingsThemeChanged = 5,
      settingsUnitChanged = 6,
      settingsFilterChanged = 7,
      heartbeat = 8;

  static DeviceEvent decode(List<int> bytes) {
    final m = (cbor.decode(bytes) as CborMap)
        .map((k, v) => MapEntry((k as CborInt).toInt(), v.toObject()));
    return DeviceEvent(m[1] as int? ?? -1, Map.from(m)..removeWhere((k, _) => k == 0 || k == 1));
  }

  HazardType? get hazardType => extra[2] != null ? HazardType.values[extra[2] as int] : null;
  int? get speedKmh => extra[3] as int?;
  bool get seen => extra[4] == true;
}

/// Persisted device settings, synced both ways over the Settings characteristic.
class DeviceSettings {
  bool soundEnabled = false;
  AudioMode soundMode = AudioMode.tones;
  bool useMph = false;
  int filterBits = 0xFFFF; // bit n = HazardType n enabled
  int nightStartHour = 20, nightEndHour = 6;
  int cacheStaleMinutes = 10;

  List<int> encode() => cbor.encode(CborValue({
        0: kProtocolVersion,
        1: soundEnabled,
        2: soundMode.index,
        3: 0, // theme (device ignores — single dark theme)
        4: useMph,
        5: filterBits,
        6: nightStartHour,
        7: nightEndHour,
        8: cacheStaleMinutes,
      }));

  static DeviceSettings decode(List<int> bytes) {
    final m = (cbor.decode(bytes) as CborMap)
        .map((k, v) => MapEntry((k as CborInt).toInt(), v.toObject()));
    return DeviceSettings()
      ..soundEnabled = m[1] == true
      ..soundMode = AudioMode.values[(m[2] as int?) ?? 0]
      ..useMph = m[4] == true
      ..filterBits = (m[5] as int?) ?? 0xFFFF
      ..nightStartHour = (m[6] as int?) ?? 20
      ..nightEndHour = (m[7] as int?) ?? 6
      ..cacheStaleMinutes = (m[8] as int?) ?? 10;
  }
}

/// Written once after connect.
List<int> encodeTime(DateTime now) => cbor.encode(CborValue({
      0: kProtocolVersion,
      1: now.millisecondsSinceEpoch ~/ 1000,
      2: now.timeZoneOffset.inMinutes,
    }));
