# Compatibility: firmware ↔ app

Two version numbers, deliberately independent (see [`release-format.md`](release-format.md)):

- **Firmware version** — SemVer of the image, reported in `DeviceInfo` key 1.
- **Protocol version** — the integer in key 0 of every CBOR message and in `DeviceInfo` key 0.
  Additive changes (new keys, new enum values appended) **do not** bump it; only a breaking change
  does (a key changes meaning or type, a field is removed).

## Rules

| Situation | What the app does |
|---|---|
| `DeviceInfo.protoVersion` equals the app's supported version | talk normally; ignore capability bits and keys it does not know |
| `protoVersion` **higher** than the app supports | do **not** stream State; ask the user to update the app. The device keeps showing its last good state |
| `protoVersion` **lower** than the app's minimum | tell the user to update the display (OTA if `hasOta` is set) |
| a capability bit is clear | see [`contract/responsibilities.md`](contract/responsibilities.md) |
| an update's manifest says `minFromVersion` is newer than the installed firmware | update in steps (first to the intermediate release) |

A receiver always ignores keys it does not know and enum values outside its range
(a `hazardType` above 10 is a generic icon; an unknown event tag is dropped) — that is what lets old
apps keep working with newer firmware inside one protocol version.

## Table

| Firmware | Protocol | Smallest app protocol | Notable | Source |
|---|---|---|---|---|
| `0.1.0` (tag `fw-v0.1.0`, first release) | 1 | 1 | all eight characteristics, OTA, asset upload, volume (Settings key 9), UiConfig; OTA transfer not yet verified on the board | GitHub Release `fw-v0.1.0` |

Rows are added per published release (the first one, `0.1.0`, is in a **private** repository for now); the manifest of each release carries the same numbers
(`firmware.version`, `protocol.version`, `protocol.minAppProtocol`).

## Additions inside protocol 1 (all additive, in the order they landed)

| Addition | Older firmware | Older app |
|---|---|---|
| UiConfig characteristic `0006` | characteristic absent | not used |
| OTA `0007`, Asset `0008`, `hasOta` cap bit | absent | not used |
| Settings key 9 `soundVolume` | ignores the key | omitting it leaves the device's stored volume |
| `DeviceInfo` caps bit 5 | not set | ignored |
