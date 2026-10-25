# App-Übergabe — Kurzfassung (Deutsch)

Vertragsstand: **1.1.0** (Protokollversion 1), Tag `protocol-v1.1.0` (neu gegenüber 1.0.0: State-Schlüssel 14
`driveActive`, additiv); Firmware-Releases `fw-v0.1.0` (Vertrag 1.0.0) und `fw-v0.2.0` (Vertrag 1.1.0).
Die ausführliche englische Fassung steht in
[`app-integration.md`](app-integration.md).

**Funktionsliste zum Abhaken:** [`app-feature-checklist.md`](app-feature-checklist.md) — jede Funktion des Displays mit dem, was die App senden/behandeln muss, plus was das Display *nicht* kann.

**Was der App-Chat bekommt:** die Spezifikation ([`ble-protocol.md`](ble-protocol.md)), die
Zuständigkeitstabelle, getestete Golden-Vektoren aus dem echten C++-Codec, die Schild- und
Kategorieregeln als JSON mit Testvektoren und ein reines Dart-Paket (`protocol/dart/`, 85 Tests
gegen genau diese Vektoren).

**Pflichten der App in Kurzform:** verbinden → DeviceInfo lesen und prüfen → Zeit schreiben → komplette
Settings schreiben → State senden (`stateSeq` ab 1, bei jeder Änderung und mindestens alle 5 s, höchstens
etwa 2/s) → Ansagen und „Gesehen?“-Anfragen wiederholt mitsenden → bei jedem Abbruch neu verbinden. Die
App entscheidet: Schildspalte und Ersetzungen, Gefahrenfilter, wann angesagt wird, Lautstärke,
Einheiten, Update-Auslieferung.

**Bekannte Einschränkungen:** Sicherheitsstufe offen (keine Verschlüsselung/Kopplung erforderlich —
auch OTA!; bewusst so entschieden), RAM auf dem Gerät knapp, „latest wins“ bei State, mögliches Reißen
beim schnellen Wischen, Sprachclips nicht im Repo.

**Entscheidungen (24.10.2026):** UUID-Basis eingefroren · Sicherheitsstufe bleibt offen · keine
Melden-Sperre · Pairing-QR zeigt auf die Repo-Seite (BLE-Name bleibt `BlitzerdeDsp`) · keine Sprachclips
im Release.

**Noch nicht am Brett geprüft:** OTA komplett, UiConfig schreiben + Neustart, Zurücksetzen, Audio ohne
Knistern, und die neuen Bildschirme „offline“ / „Leerlauf mit TNV-Logo“ (bisher nur im Simulator) — der Prüfplan steht in [`testing.md`](testing.md), Abschnitt „Board verification“.
