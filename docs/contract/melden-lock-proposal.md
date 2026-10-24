# Proposal: let the app lock the Melden (report) page while driving

**Status: WITHDRAWN (2026-10-24) — the maintainer does not want a report lock; nothing was built.** The text is
kept as a record (it documents how the persistent UiConfig bit behaves today). Everything in it was
additive and backwards compatible with protocol v1.

## What exists today

The Melden page is a touch UI (rotary crown, 11 categories) reachable by a swipe or the "+"
button. The only way for the app to remove it is the persistent **UiConfig** characteristic
(`0006`), bit 0 `meldenPage`. Checked in the code (`core/state/`, `ui/screens/`):

| Behaviour | Result |
|---|---|
| Clear bit 0 | "+" is hidden, the swipe into Melden is locked, `open_melden()` does nothing |
| Page already open when the bit is cleared | **stays open** until the 15 s idle timeout (any touch resets it) — it is not evicted |
| Persistence | every write goes to NVS and survives a reboot; if the app crashes or never re-enables it, the page stays off |
| Intent | documented as a *developer feature switch*, not a driving lock |

So a driving lock is *possible* with UiConfig but awkward: it wears flash if toggled per drive
state, can strand the user without the page, and does not close an open page.

## Proposed addition

A **non-persistent, per-State-packet flag** in `DisplayState` (State characteristic `0001`):

| Key | Type | Field | Meaning |
|---|---|---|---|
| `flags1` bit 6 | bit | `reportLocked` | 1 = the report page is not available right now |

Device behaviour when set (all UI-state logic, no computation — the app decides *when*):

1. "+" is hidden/disabled; swipe entry into Melden is blocked (same path as `meldenPage = 0`).
2. If Melden is open, return to Home immediately; **no `ReportSubmitted` event** is emitted.
3. Not written to NVS and **not part of the offline cache** — it lives only in the live State.
4. Absent bit = 0 = unlocked, so existing apps and the PC test tool keep working unchanged.
   Older firmware ignores unknown bits (the protocol's forward-compatibility rule).

## Decisions needed from the maintainer

1. **Should the lock apply to the display at all?** (The app has its own driving lock.)
2. **Fail-safe when the link drops** while locked: (a) stay locked until the app says otherwise,
   (b) unlock because there is no live state (current cache behaviour), (c) lock whenever the link
   is down. Safety argues for (a) or (c); usability for (b).
3. Should `confirm` ("Gesehen?") prompts also be suppressible while driving? (Today the prompt is
   always shown when the app asks.)

## Cost

Small: one bit in `core/codec/protocol.cpp` (flags1), one field in `DisplayState`, one branch in
`AppStateMachine::recompute()` plus a host test, and a line in `ble-protocol.md`. No wire
incompatibility. Until it exists, the interim option is UiConfig bit 0, with the caveats above.
