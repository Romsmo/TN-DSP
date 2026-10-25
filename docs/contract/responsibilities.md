# Who does what

The display **renders; it never computes** (docs/architecture.md). Everything that needs a decision
about the road, the user or the data belongs to the app.

| Topic | The app | The display |
|---|---|---|
| Current speed limit | decides the value and unit; sends `speedLimit` + `speedLimitUnit` (a **sign value**, shown unchanged — a UK "30 mph" sign stays 30) | draws it; never converts |
| Hazard warnings | decides *whether and when* (`hazardActive`, `hazardType`, `hazardDistanceM`, `hazardSpeedLimit`); only sends camera categories (ids 0–4) the user switched on | draws the overlay; as a second layer it suppresses hazards of categories that are off in `filterBits` |
| Category filter | owns the user's choice; writes `filterBits` (Settings key 5) | stores it, applies it as above; unknown/future types are always shown |
| Context signs | picks which signs exist, which **column** each goes in and which signs **replace** each other ([`sign-rules.json`](sign-rules.json)); sends ≤ 4 per column, oldest first | renders the two lists as received (newest 3 per column); unknown codes show an empty slot |
| Trip (Fahrt) | decides when a trip runs; sends `driveActive = false` while connected but idle and keeps sending State | idle: shows the TNV logo and only Settings; offline (no app): black, only Settings; driving: the full UI |
| Free-road picture | sets `directionArrows` | draws it (hidden while a hazard overlay is up) |
| Confirm prompt ("Gesehen?") | decides when; sets `confirmPromptActive` + timeout | shows it, counts down, sends `ConfirmResponse` or times out silently |
| Report (Melden) | receives `ReportSubmitted` and decides what to do with it; may strip the page with `UiConfig` bit 0 | shows the crown, sends the event; **does not lock while driving** (a lock was considered and dropped: [`melden-lock-proposal.md`](melden-lock-proposal.md)) |
| Audio | decides *when* to announce: sends an `audioCue` with a new `seq`; owns sound on/off, mode and volume (Settings) | decides *how it sounds*: picks the chime or composes the phrase from the clips on its card; degrades to the chime if any clip is missing |
| Wall clock | writes `Time` after every connect | keeps it in the RTC |
| Settings | writes the complete set after connect; reflects changes the user makes on the device (notify) | persists them (NVS) |
| UiConfig | reads it; changes it only deliberately (persistent, developer switches) | persists and applies it |
| Night dimming | sets `nightMode` (the theme is always dark) | dims the panel only |
| Keep-alive and `stateSeq` | restarts at 1 per connection, strictly increasing, State at least every 5 s | drops stale/duplicate packets, shows the "no app" badge after 8 s, renders from cache |
| Offline cache | — | keeps the last State across ignition cycles |
| Firmware updates | finds a new release, checks its signature and manifest, pushes the image over `0007`, waits for the reboot, verifies `fwVersion` | verifies SHA-256, switches slot, rolls back an image that does not confirm itself |
| Voice clips | owns the delivery of the audio pack and pushes files over `0008` | stores them on the SD card; works without them |
| Factory reset | learns of it only by the changed connection (bonds are gone) | wipes settings, cache, UiConfig and bonds on the user's confirmation |

## What the app does when a capability bit is missing (DeviceInfo key 2)

| Bit | If it is **clear** |
|---|---|
| `0 hasAudio` | never send an `audioCue`; hide sound settings |
| `1 hasImu` | nothing (no feature uses it yet; the bit is not set today) |
| `2 hasRtc` | still write `Time` (harmless); do not rely on the display keeping time across power loss |
| `3 hasSD` | no card mounted: do not offer voice mode or asset upload; the display plays chimes |
| `4 speedcamBuild` | this firmware has no speed-camera code: hide categories 0–4 in the filter UI and never send them as hazards |
| `5 hasOta` | do not offer a firmware update |

A **protocol major version different from the app's** → do not stream State; tell the user to update
the app or the display (see `ble-protocol.md`, *Version compatibility*, and
[`../compatibility.md`](../compatibility.md)).
