# Release format (the interface to the app)

**Status: specification and draft. No release, tag or upload has been made — each needs the
maintainer's explicit yes (see "Publishing").**

A release is what the phone app looks for when it checks for a firmware update. Everything the app
needs is in **`manifest.json`**; the rest are the files it points to.

## Files of a release

| File | Contents |
|---|---|
| `tn-display-<version>.bin` | the **OTA image** (the ESP-IDF app image, `blitzerdsp.bin` from the build) |
| `tn-display-<version>.sha256` | `sha256sum`-style line for the image |
| `manifest.json` | machine-readable description, versioned, validated by [`contract/manifest.schema.json`](contract/manifest.schema.json); example: [`contract/manifest.example.json`](contract/manifest.example.json) |
| `manifest.json.sig` | detached signature of the manifest — **planned** (option G in [`update-authenticity.md`](update-authenticity.md)); not produced yet |
| `audio-pack-<version>.zip` (+ `.sha256`) | optional: the voice clips, if the maintainer decides to ship any (see below) |
| `THIRD_PARTY.md` + `licenses/` | notices that must accompany the binary |

The `tn-display` stem is a placeholder until the project name is decided; the CI job currently
produces the image as `blitzerdsp.bin`.

## `manifest.json`

Versioned by `manifestVersion` (an additive change does not bump it; readers ignore unknown fields).

| Field | Type | Meaning |
|---|---|---|
| `manifestVersion` | int | `1` |
| `firmware.version` | string | SemVer of the firmware, e.g. `0.2.0` (same string the device reports in `DeviceInfo` key 1) |
| `firmware.gitCommit` | string | commit the image was built from |
| `firmware.image` | object | `{file, sizeBytes, sha256}` — size in bytes (the OTA slot holds 3 MiB) and the SHA-256 the app sends in the OTA `begin` frame |
| `firmware.minFromVersion` | string | the lowest installed firmware version from which a direct OTA update to this one is allowed; older devices must update in steps |
| `protocol.version` | int | BLE protocol major version the firmware speaks (`DeviceInfo` key 0) |
| `protocol.minAppProtocol` | int | the smallest app-side protocol version that can drive this firmware |
| `releasedAt` | string | ISO-8601 date |
| `changes` | object | `{de: [..], en: [..]}` — short change lists |
| `audioPack` | object \| null | `{included: bool, version, file, sha256, sizeBytes}`; `null`/`included:false` = this release ships no clips |
| `signature` | object \| null | reserved for the planned manifest signature `{alg, keyId, file}` |

The app trusts `firmware.image.sha256` only after it has verified the manifest signature (once that
exists) — see the authenticity note.

## Version rules

- **Firmware version** = SemVer of the image. **Protocol version** = the integer in key 0 of every
  CBOR message. They are independent: a firmware release can change the UI without touching the
  protocol, and the protocol only bumps its *major* on a breaking change.
- [`compatibility.md`](compatibility.md) lists which firmware speaks which protocol and which app
  protocol range can drive it.
- The release tag for a firmware image is `fw-vX.Y.Z`; for the contract `protocol-vX.Y.Z`
  (proposed names, nothing is tagged yet).

## Audio pack

The German voice clips are **not** in the repository (see `THIRD_PARTY.md`, section 6). The device
works without them. If the maintainer later decides to ship a pack made from recordings with clear
rights, it is a separate release asset with its own checksum; the app pushes it over the Asset
characteristic (`0008`) after an update, and `categories.json` lists the exact files and paths.

## How the app uses a release

1. Fetch `manifest.json` (latest release of the repository — public repo required for an
   unauthenticated app).
2. Compare `firmware.version` with `DeviceInfo.fwVersion`; check `minFromVersion` and
   `protocol.minAppProtocol` against the app's own protocol version.
3. Download the image, check size and SHA-256 against the manifest (and, when it exists, the manifest
   signature), then push it over `0007` as described in `ble-protocol.md`.
4. After the reboot, read `DeviceInfo` and confirm the new `fwVersion`.

## Publishing

Creating a tag, a GitHub Release or uploading assets happens **only on the maintainer's explicit
yes, per release**, and only after the on-board update test (D4 in `docs/testing.md`) has passed.
The CI job already builds the image and a checksum as a workflow artifact; turning that into a
release (`gh release create …`) is a manual, approved step.
