# Firmware update authenticity — options and recommendation

**Research note. Nothing here has been implemented, and nothing irreversible will be done without
the maintainer's written approval.** Sources are the ESP-IDF 5.3.2 documentation shipped in the
toolchain used for this project (`docs/en/security/secure-boot-v2.rst`, `flash-encryption.rst`,
`docs/en/api-reference/system/ota.rst`); quoted line references are to that tree.

## Where we are

The OTA characteristic accepts an image from **any** BLE central in range (no pairing is required,
see `ble-protocol.md`, *Security level*). The `begin` frame carries a SHA-256 — but it is supplied
by the sender, so it only proves the bytes arrived intact, **not who made them**. Signing the release
manifest in the app does not change that on its own: a connected attacker can bypass the app and
write to `0007` directly. Real protection has to be enforced **on the device**.

## What the ESP32-C6 / ESP-IDF 5.3.2 offer

| Option | Protection | Effort | Irreversible? | Risk |
|---|---|---|---|---|
| **A. Status quo** (SHA-256 from the sender) | transfer errors only | none | no | any central can flash any image |
| **B. Require an encrypted, bonded link** for OTA/Asset/State/Settings writes (NimBLE GATT flags `*_ENC`, optionally `*_AUTHEN` with a passkey on the AMOLED) | stops casual and drive-by access; with passkey also active MITM during pairing | small–medium: firmware + pairing UX (iOS pairs automatically on first protected access); stale-bond handling after a device reset | **no** | does not authenticate the *image*; a paired phone with a bad app can still flash |
| **C. Signed app images without hardware Secure Boot** ("Signed App Verification Without Hardware Secure Boot", `CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT`) | the running app verifies the Secure Boot V2 signature of a *new* image before accepting an OTA (RSA-3072 or ECDSA on the C6); an attacker without the private key cannot make an accepted image | medium: generate a key pair (kept offline), sign in the release job, first image must already be signed (secure-boot-v2.rst, "Signed App Verification Without Hardware Secure Boot") | **no** — no eFuse is burned; can be switched off by a new build | does **not** stop an attacker who can write to flash (physical access, or a bad image flashed over USB) — the doc states this explicitly; trust chain starts at the *currently running* signed app; only the first public key in the signature block is used |
| **D. Secure Boot V2** (hardware, key digest in eFuse) | the ROM verifies bootloader and app signatures on every boot; strongest against flash writes | high: key management, signed bootloader, production process | **YES** — public-key digest eFuses are one-time; unused key slots must be revoked before the device leaves the factory (secure-boot-v2.rst, key revocation); a wrong key or aggressive revocation can brick the device (secure-boot-v2.rst, "this could also brick the device permanently") | needs a recovery plan; UART download mode choices ("Permanently switch to Secure mode") are one-way |
| **E. Flash encryption** (hardware, key in eFuse) | confidentiality of the image at rest; with Release mode only OTA can write plaintext | high | **YES** (key burned and read/write-protected; Release mode burns download-mode-disable bits) | development becomes harder; combine with D, not instead of it |
| **F. Anti-rollback** (`CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK`, security version in eFuse) | prevents downgrading to an older, vulnerable image | small once D/C exist | **YES** — the eFuse security version only ever increases (ota.rst, "Anti-rollback") | a bad version bump can lock out good images |
| **G. Manifest signature checked by the app** (e.g. Ed25519 / ECDSA over `manifest.json` + image hash, public key embedded in the app, private key offline) | protects the *distribution channel* (a tampered GitHub release or a man-in-the-middle on download) | small–medium; needs a key ceremony | no | does not protect the device from a malicious central — it only keeps honest apps from installing a tampered download |

## Recommendation

1. **Do B now** (encrypted/bonded link, passkey shown on the display): it is reversible and closes
   the open door that matters most for a device in a car. It needs a maintainer decision on the pairing
   UX because it changes the first-connect flow.
2. **Do C and G together** for the first public releases: sign the image in the release job (key
   offline, never in CI secrets without your approval), let the device verify it on OTA (C) and let the
   app verify the manifest signature before offering an update (G). Both are reversible by shipping a
   new build.
3. **Defer D, E and F.** They are eFuse operations. Propose them again only when a production run is
   planned, with a written recovery plan (USB serial remains available until UART download mode is
   permanently restricted) — and only after your explicit written approval, naming the irreversibility.

`TODO(verify)`: the exact Kconfig names and the C6 signing scheme choice (RSA-3072 is faster to verify
than ECDSA on the C6 per the ESP-IDF docs, ≈ 2.7 ms vs ≈ 21.5 ms) have to be confirmed against a real
build before implementation. Key generation is documented in the ESP-IDF Secure Boot V2 guide
(`espsecure.py generate_signing_key --version 2 …`, or openssl for production keys) and has **not**
been run here; production keys must be generated and kept offline by the maintainer.
