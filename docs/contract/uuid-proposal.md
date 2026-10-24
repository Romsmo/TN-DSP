# Proposal: the final UUID base

**Status: DECIDED 2026-10-24 — the current base is final for protocol v1** (the maintainer delegated the choice; the recommendation below was adopted). It was provisional until then:

```
079aXXXX-d673-469c-98a0-12c0c5eb43b3        XXXX = 16-bit slot, 0000 = service, 0001..0008 = characteristics
```

## Recommendation: freeze the current base

1. **No registration is needed.** A vendor-specific 128-bit UUID needs no entry with the Bluetooth
   SIG. (16-bit UUIDs are SIG-assigned numbers and must not be invented; the SIG base
   `0000xxxx-0000-1000-8000-00805f9b34fb` must not be reused for custom slots.)
2. **It is already random.** The base was generated randomly for development (comment in
   `ble_uuids.h`); a random 128-bit value is collision-safe for this purpose, and it contains no
   brand, product or person name — so a rename of the project does not make it wrong.
3. **A change later breaks field devices.** The service UUID is in the advertising packet and apps
   scan for it. A changed base means a new app cannot find old displays (and the reverse). Deciding
   now costs nothing; deciding after the first release costs a migration.
4. **Cost of keeping it:** none — it is already in the firmware, `tools/fake_app.py`, the golden
   tests (`test/ble_uuids_test.cpp`) and the Dart package.

## If you prefer a new base

Pick it **before** the first public release. Generate with any UUIDv4 tool, then change it in one
place per language and re-run the tests:

| Where | What |
|---|---|
| `firmware/components/transport_ble/ble_uuids.h` | the byte array in `BDSP_BLE_UUID128` (little-endian) |
| `protocol/dart/lib/src/constants.dart` | `uuidBasePrefix` / `uuidBaseSuffix` |
| `tools/fake_app.py` | `_uuid()` |
| `test/ble_uuids_test.cpp`, `docs/ble-protocol.md` | expected values / the table |

One non-technical note: the comment above the base in `ble_uuids.h` used to say it might change
"during licensing talks" with a third party. That sentence has been removed from the code; if such a
vendor base is still wanted, the decision belongs here, before freezing.
