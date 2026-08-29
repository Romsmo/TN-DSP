# Projektplan: Eigenes Blitzer.de-Display für BMW E90 (Open Source)

## 1. Ziel

Ein eigenes, quelloffenes Hardware-Display fürs Auto (E90) — im Grunde die Blitzer.de-Pro-App "vom Handy aufs Dashboard ausgelagert". Besteht aus einer Flutter-App (iOS + Android, API-Anbindung, Logik) und einem ESP32-AMOLED-Board (reine Anzeige + Touch-Interaktion). Datenbasis ist vollständig Blitzer.de, sobald der offizielle API-Zugang vorliegt.

**Lizenz-/Vertriebsmodell (mit Blitzer.de/Eifrig Media abzustimmen):**
- Projekt wird open source gestellt, Nachbau durch Dritte ausdrücklich gewünscht
- App-Veröffentlichung wahlweise über eigenen Account oder durch Eifrig Media selbst
- Ziel: kostenlose Nutzung für bestehende Blitzer.de-Pro-Abonnenten
- Abdeckung: europaweit, 1:1 wie Blitzer.de Pro

---

## 2. Hardware

- **Display:** ESP32-S3 Touch-AMOLED-Board (Waveshare-Serie, genaue Variante 1.43"/1.75"/1.8"/1.91"/2.06" noch zu bestätigen)
  - Nur 1.75" und 2.06" haben onboard Audio-Codec (ES8311) mit Lautsprecherausgang
- **Verbindung zum Handy:** Bluetooth Low Energy (BLE)
- Kein GPS-Modul am ESP32 nötig — Position kommt vom Smartphone

---

## 3. Architektur / Datenfluss

```
[Smartphone: Standortdienste (Flutter geolocator)]
        │
        ▼
[Blitzer.de API]  →  Blitzer, Gefahrenstellen, Meldungen/Bestätigungen, Live-Updates
        │
        ▼
[Flutter App: State-Verwaltung + Settings-Filter + Straßenbezogenes Map-Matching]
        │  (BLE, Notify)
        ▼
[ESP32 + AMOLED: reine Anzeige, LVGL]
        │  (BLE, Events)
        ▲
[Touch-Interaktion: +, Swipe, Bestätigung]
```

---

## 4. Feature-Abgleich mit Blitzer.de Pro

Recherchierter Funktionsumfang von Blitzer.de (Basis + Pro), abgeglichen mit dem aktuellen Plan:

| Blitzer.de-Feature | Im Projekt enthalten? | Anmerkung |
|---|---|---|
| Feste Blitzer (weltweit/europaweit) | ✅ Ja | Kernfunktion, Datenquelle Blitzer.de API |
| Aktive mobile Blitzer | ✅ Ja | Kernfunktion |
| Gefahren (Stau, Unfall, Hindernis, Glätte, Baustelle, Panne) | ✅ Ja | Bereits in Screen-Konzept enthalten |
| Melden & Bestätigen | ✅ Ja | "+"-Screen + "Gefahr gesehen?"-Prompt |
| Optische + akustische Warnung | ✅ Ja | AMOLED-Anzeige + Lautsprecher (bei Board-Variante mit Audio-Codec) |
| Automatische Live-Updates | ✅ Ja | Laufender API-Abruf während der Fahrt |
| Entfernungsanzeige zum Blitzer/zur Gefahr | ✅ Ja | Bereits im Payload (`hazardDistanceM`) |
| **Offline-Modus** | ⚠️ Nachgeschärft | Bisher nur "kein Internet"-Anzeige geplant. Sollte zusätzlich zuletzt bekannte Daten lokal cachen, statt nur leer anzuzeigen |
| **Straßenbezogene Warnung (Map-Matching, weniger Fehlalarme)** | ⚠️ Nachgeschärft | War beim TomTom-Ansatz mitgedacht, muss jetzt explizit auch für Blitzer.de-Positionsdaten umgesetzt werden (GPS-Punkt korrekt der befahrenen Straße zuordnen, nicht nur Luftlinie) |
| Sprachansagen (TTS) | ✅ Ja | Bereits geplant, über Handylautsprecher/Bluetooth |
| **Hintergrundbetrieb** | ⚠️ Neu ergänzt | App muss Standort/Warnungen auch bei gesperrtem Bildschirm/im Hintergrund weiterverarbeiten (iOS Background Location, Android Foreground Service) |
| **Automatischer Start beim Einsteigen** | ⚠️ Neu ergänzt | Z. B. Auto-Connect-Trigger, sobald BLE-Verbindung zum ESP32 erkannt wird |
| Personalisierung (welche Blitzer-/Gefahrentypen aktiv) | ✅ Ja | Bereits als Settings-Toggles geplant |
| **Tag-/Nacht-Modus** | ⚠️ Neu ergänzt | Display-Helligkeit/Farbschema je nach Tageszeit, bisher nicht explizit im Plan |
| **Geschwindigkeitseinheit (km/h ↔ mph)** | ⚠️ Neu ergänzt | Kleine Ergänzung in Settings |
| Kein Werbung, persönlicher Support | – | Betrifft Blitzer.de als Anbieter, nicht das Display-Projekt selbst |
| Kartenansicht / Mini-App-Overlay / Android Auto | ❌ Nicht übertragbar | Diese Pro-Features setzen ein normales Smartphone-Display voraus. Unser Board hat ein kleines, dediziertes rundes/eckiges Display ohne Kartenansicht — funktional durch die Live-Anzeige (Schild + Distanz + Warnung) ersetzt, nicht 1:1 nachgebaut |
| Vibration für Motorradfahrer | ❌ Nicht relevant | Zielplattform ist das Auto-Dashboard, nicht am Körper getragen |

**Fazit:** Die warnungs- und datenrelevanten Kernfunktionen sind vollständig abgedeckt. Vier Punkte (Offline-Caching, Map-Matching, Hintergrundbetrieb, Auto-Start) waren im bisherigen Plan nicht explizit genug und sind jetzt ergänzt. Reine Smartphone-UI-Features (Kartenansicht, Mini-App, Android Auto) lassen sich auf einem dedizierten Kleindisplay konzeptionell nicht 1:1 übertragen, sind für den Zweck aber auch nicht nötig.

---

## 5. Rechtlicher Hinweis

§23 Abs. 1b StVO verbietet die Nutzung von Geräten/Apps, die während der Fahrt vor Verkehrsüberwachungsmaßnahmen (Blitzer) warnen. Da das Projekt zentral auf Blitzer.de-Daten (inkl. Blitzer-Kategorien) aufbaut, betrifft das den Kern der App. Die Trennung im UI zwischen Blitzer-Kategorien und allgemeinen Gefahrenkategorien (Stau/Glätte/Unfall/etc., unkritisch) bleibt wichtig.

---

## 6. Screens / State Machine (Basis: Skizze)

| State | Trigger | Inhalt |
|---|---|---|
| **Home** | Default | Verbindungsstatus-Icon, Tempolimit-Schild, Richtungspfeile (grün = frei), Überholverbot-Icon, "+"-Button, Settings-Zahnrad |
| **Kein Internet** | Automatisch bei Verbindungsverlust | Zuletzt bekannte Daten (gecacht) + Schild + Überholverbot |
| **Blitzer/Gefahr in der Nähe** | Automatisch bei Annäherung | Blinkendes Warn-Icon + Distanz-Countdown, Tempolimit bleibt sichtbar |
| **Melden** | Swipe links ODER "+"-Druck | Kategorien: Blitzer (Fest/Mobil/Anhänger/Rotlicht/Abstand, inkl. Speed-Eingabe bei mobil) und Gefahr (Stau/Glätte/Unfall/Baustelle/Panne/Hindernis) |
| **Gefahr bestätigen** | Getriggert durch die Blitzer.de-API, nicht lokal berechnet | "Gefahr gesehen?" Ja/Nein, 15 Sek. Timeout zurück zu Home |
| **Settings** | Swipe hoch | Handy verbinden, Ton An/Aus, Tag-/Nacht-Modus, Einheit (km/h/mph) |
| **Handy verbinden** | Automatisch bei fehlender BLE-Verbindung | Pairing-Aufforderung |

---

## 7. BLE-Kommunikationsprotokoll

### 7.1 App → ESP32 (Notify, State-Payload)

```dart
class DisplayPayload {
  bool internetConnected;

  int? speedLimit;          // null = kein Schild aktiv
  bool overtakingBan;
  bool directionArrows;     // grüne Pfeile "frei"

  bool hazardActive;
  HazardType? hazardType;
  int? hazardDistanceM;
  int? hazardSpeedLimit;    // bei mobilen Blitzern

  bool confirmPromptActive;
  int confirmPromptTimeoutS = 15;

  bool nightMode;
  bool useMph;
}

enum HazardType {
  fixedSpeedCamera, mobileSpeedCamera, trailerCamera,
  redLightCamera, distanceControl,
  traffic, ice, accident, construction, breakdown, obstacle,
}
```

### 7.2 ESP32 → App (Events, Nutzerinteraktionen)

```dart
sealed class DeviceEvent {}
class ReportPressed extends DeviceEvent {}
class ReportSubmitted extends DeviceEvent {
  final HazardType type;
  final int? speedKmh;
  ReportSubmitted(this.type, this.speedKmh);
}
class ConfirmResponse extends DeviceEvent {
  final bool seen;
  ConfirmResponse(this.seen);
}
class SettingsPhoneConnect extends DeviceEvent {}
class SettingsSoundToggle extends DeviceEvent {
  final bool on;
  SettingsSoundToggle(this.on);
}
```

Prinzip: ESP32 spricht nie direkt mit der Blitzer.de-API — alle Datenanfragen laufen über die App, das Board ist reine Anzeige- und Interaktionsschicht.

---

## 8. Offene Punkte

- [ ] **Blockierend:** Rückmeldung/Lizenz-/Preisvorstellung von Blitzer.de (Sebastian, Eifrig Media) abwarten
- [ ] Klären, ob die API Schreibzugriff (Melden/Bestätigen) für Drittanbieter-Apps überhaupt vorsieht
- [ ] Technische Klärung mit Blitzer.de: wie würde eine "kostenlos für Pro-Abonnenten"-Gate technisch aussehen (z. B. Login/OAuth über Blitzer.de-Account zur Lizenzprüfung)?
- [ ] Rate Limits / Polling-Intervall vs. Push-Möglichkeiten
- [ ] Map-Matching-Ansatz festlegen (eigene Lösung vs. ggf. von Blitzer.de bereitgestellt)
- [ ] Exakte Board-Variante bestätigen (Displaygröße, Audio-Codec vorhanden ja/nein)
- [ ] BLE-Service/Characteristic-UUIDs festlegen
- [ ] Open-Source-Lizenz wählen (z. B. MIT/Apache für Code — ggf. Einschränkungen durch Blitzer.de bzgl. API-Nutzung/Branding klären)

---

## 9. Nächste Schritte (Reihenfolge)

1. Auf Lizenz-/Preisvorstellung von Blitzer.de warten
2. Sobald Docs vorliegen: Datenmodell/Endpoints mit Abschnitt 4 (Feature-Abgleich) gegenprüfen
3. BLE-Grundgerüst ESP32 (Peripheral, Service/Characteristics gemäß Protokoll oben) — unabhängig von der API-Zusage startbar
4. LVGL-Screens für Home + Kein-Internet-State (mit Dummy-Daten testen)
5. Flutter-App-Grundgerüst: Standort, Hintergrundbetrieb, BLE-Client
6. Blitzer.de-API-Integration inkl. Map-Matching, sobald verfügbar
7. Restliche Screens (Melden, Bestätigen, Settings inkl. Tag/Nacht + Einheit) fertigstellen
8. Open-Source-Repo aufsetzen (Lizenz, Doku für Nachbauer)
