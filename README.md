# ESP32 LED Uhr mit Websteuerung

![ESP32 LED Uhr Hardware](images/hardware.jpg)

Dieses Projekt steuert bis zu 3x 8x8 WS2812B-LED-Panels mit einem ESP32.  
Es bietet eine Weboberfläche zum Einstellen von:
- Anzeige-Modus (Uhr, Regenbogen, feste Farbe, Lauftext)
- Farbe und Helligkeit
- Text für den Lauftext

## Hardware
- ESP32 DevKit v1
- WS2812B LED-Panels (3× 8x8, in Reihe verbunden)
- Netzteil 5V (mind. 3A empfohlen)
- Jumperkabel

## Schaltplan

ESP32 Pin 15 → DIN erstes Panel
5V → 5V Panel
GND → GND Panel


## Installation
1. **Arduino IDE installieren**  
   [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

2. **ESP32-Board-Paket installieren**  
   - Datei → Voreinstellungen → zusätzliche Boardverwalter-URLs:  
     `https://dl.espressif.com/dl/package_esp32_index.json`  
   - Werkzeuge → Board → Boardverwalter → „esp32“ installieren.

3. **Bibliotheken installieren**  
   - `Adafruit_NeoPixel`
   - `WiFi`
   - `NTPClient`
   - `TimeLib`

4. **Code hochladen**  
   - Sketch in Arduino IDE öffnen.  
   - WLAN-Name und Passwort im Code eintragen.  
   - Board: **ESP32 Dev Module** auswählen.  
   - Port wählen.  
   - Hochladen klicken.

5. **Weboberfläche öffnen**  
   - ESP32 verbindet sich mit WLAN → IP im Seriellen Monitor auslesen.  
   - Diese IP im Browser öffnen → Steuerseite erscheint.

## Bedienung
- **Mode**: Anzeige-Modus wählen
- **Set Color**: Farbe einstellen (bei „Solid“ oder Lauftext)
- **Set Brightness**: Helligkeit anpassen
- **Scrolling Text**: Text für Lauftext-Modus

## Lizenz
MIT License – frei zur Nutzung.
