# Legal notice — DRAFT for legal review

**Status: draft written by the maintainer's tooling, not reviewed by a lawyer. Nothing here is legal
advice.** It exists so the repository does not go public without a considered statement. Items
marked `TODO(verify)` must be confirmed by a qualified person.

## Short form (also in the README)

This device only displays what its companion app sends. It can show speed-camera and hazard
warnings if — and only if — the app is told to send them, which the user has to switch on
explicitly. In several countries, including Germany, using a device that is designed to warn of
traffic enforcement while driving is prohibited; other countries have stricter or different
rules. The firmware can be built without any speed-camera code. You are responsible for obeying
the law where you drive.

## Points the review should cover

1. **Germany — § 23 StVO.** The prohibition on operating or carrying a technical device meant to
   *indicate or disrupt traffic-enforcement measures* applies to the driver. `TODO(verify)`: the
   exact paragraph and wording as currently in force (the README used to cite "§ 23 (1b)"; that
   citation was not confirmed and has been removed), and whether a *display* that merely renders
   data supplied by a phone app falls under it when speed-camera categories are switched on.
   The firmware's design position: the device never decides to warn; the app does, per the user's
   explicit opt-in.
2. **Other countries.** Several European countries ban or restrict speed-camera warning
   (including via smartphone apps). The compile-time switch `CONFIG_BDSP_FEATURE_SPEEDCAM=n`
   produces a build with no speed-camera types, icons or clips for jurisdictions that require it.
   `TODO(verify)`: whether the repository should recommend that build as the default for
   distribution.
3. **Interaction while driving.** The device has a touch "Melden" (report) page. Reporting while
   driving is a distraction risk and may itself be restricted. The protocol currently has **no
   field with which the app can lock the Melden page**; the app's own driving lock does not
   automatically apply to the display. A backwards-compatible extension is proposed in
   `docs/contract/melden-lock-proposal.md` — the decision is the maintainer's.
4. **No warranty / no safety function.** The device is an information display, not a driver-
   assistance system. Speed-limit and hazard data come from the app and may be wrong, late or
   missing. MIT license disclaimer applies; consider an explicit "not for safety-critical use"
   sentence. `TODO(verify)`.
5. **Names and marks.** Product/repository naming and any third-party brand appearing in the
   name, documentation or the pairing URL — see the maintainer's naming decision.
6. **Open-source obligations for binary releases.** Releases contain Apache-2.0, MIT, BSD-style
   (FatFs) code; the notices must accompany the binary (`THIRD_PARTY.md` section 2).
