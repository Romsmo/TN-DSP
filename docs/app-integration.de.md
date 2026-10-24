# App-Übergabe — Kurzfassung (Deutsch)

Vertragsstand: **1.0.0-draft** (Protokollversion 1). Vorgeschlagener Tag: `protocol-v1.0.0` — **noch
nicht gesetzt**, dafür brauche ich dein Ja. Die ausführliche englische Fassung steht in
[`app-integration.md`](app-integration.md).

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
auch OTA!), UUID-Basis noch vorläufig, RAM auf dem Gerät knapp, „latest wins“ bei State, mögliches
Reißen beim schnellen Wischen, Sprachclips nicht im Repo.

**Offene Entscheidungen von dir:** (1) UUID-Basis einfrieren? (2) Sicherheitsstufe: verschlüsselte
Verbindung, Passkey auf dem Display, signierte Images? (3) Sperre der Melden-Seite durch die App?
(4) Name, BLE-Gerätename und Pairing-URL. (5) Sprachclips: aufnehmen/lizenzieren oder keine mitliefern.

**Noch nicht am Brett geprüft:** OTA komplett, UiConfig schreiben + Neustart, Zurücksetzen, Audio ohne
Knistern — der Prüfplan steht in [`testing.md`](testing.md), Abschnitt „Board verification“.
