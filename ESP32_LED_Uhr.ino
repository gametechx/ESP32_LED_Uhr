#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// --- LED Panel Konfiguration ---
#define LED_PIN 13 // Datenpin für die WS2812B LEDs (GPIO13)
#define NUM_LEDS_PER_PANEL 64 // 8x8 Matrix
#define NUM_PANELS 3 // WICHTIG: 3 PANELS
#define TOTAL_LEDS (NUM_LEDS_PER_PANEL * NUM_PANELS)

CRGB leds[TOTAL_LEDS];

// --- WLAN Konfiguration (Feste Werte für STA und AP) ---
// Daten für dein Heim-WLAN (Client-Modus / Station Mode)
const char *STA_SSID = "Dein_Wlan_Name";       // Deine Heim-WLAN SSID
const char *STA_PASSWORD = "Dein_Wlanpasswort";    // Dein Heim-WLAN Passwort

// Daten für den ESP32 Access Point (AP Mode)
const char *AP_SSID = "ESP32_LED_Uhr";        // Name des WLANs, das der ESP32 eröffnet
const char *AP_PASSWORD = "Dein_Passwoet_fur_die_uhr";           // Passwort für den ESP32 AP

WebServer server(80); // Webserver auf Port 80

// --- NTP Konfiguration ---
WiFiUDP ntpUDP;
// NTP Server, GMT Offset in Sekunden (MESZ = GMT+2 = 7200 Sekunden), Update-Intervall (ms)
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000);

// --- Anzeigeeinstellungen ---
enum DisplayMode {
  CLOCK,
  RAINBOW_CYCLE,
  SOLID_COLOR,
  SCROLLING_TEXT
};
DisplayMode currentMode = CLOCK;

CRGB solidColor = CRGB::White; // Standard-Farbe für "Feste Farbe"
uint8_t rainbowHue = 0; // Für Regenbogen-Animation

// --- Variablen für Helligkeit und Ziffernfarbe ---
uint8_t currentBrightness = 50; // Standard-Helligkeit (0-255)
CRGB clockDigitColor = CRGB::Red; // Standard-Farbe der Uhrziffern
CRGB scrollingTextColor = CRGB::Blue; // Farbe für Lauftext

// --- Variablen für Lauftext ---
String scrollingText = "Hallo Welt! :)"; // Standard-Lauftext
int scrollOffset = 0; // Start-Offset für den Lauftext
unsigned long lastScrollTime = 0;
const int scrollSpeed = 100; // ms pro Pixel-Bewegung (schneller scrollt es)

// Font für Zeichen (5x7 Pixel)
// DIESES ARRAY WURDE VON UNTEN NACH HIER VERSCHOBEN!
const static byte ASCII_FONT[96][7][5] = {
  // From ! (33) to ~ (126) - Total 94 characters, plus ' ' and 'DEL' (127, not used here)
  // Adjusted to 96 to cover ASCII 32 to 127, which aligns with common font tables.
  // Each char is 5x7 pixels
  // ' ' (space) (ASCII 32)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // ! (33)
  {{0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,1,0,0}},
  // " (34)
  {{0,1,0,1,0}, {0,1,0,1,0}, {0,1,0,1,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // # (35)
  {{0,0,1,0,0}, {0,1,1,1,0}, {0,0,1,0,0}, {0,1,1,1,0}, {0,1,1,1,0}, {0,0,1,0,0}, {0,0,1,0,0}},
  // $ (36)
  {{0,0,1,0,0}, {0,1,1,1,0}, {1,0,0,0,0}, {0,1,1,1,0}, {0,0,0,0,1}, {0,1,1,1,0}, {0,0,1,0,0}},
  // % (37)
  {{1,0,0,0,1}, {1,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {0,0,1,0,0}, {0,1,0,0,1}, {1,0,0,0,1}},
  // & (38)
  {{0,1,1,1,0}, {1,0,0,0,1}, {0,1,1,0,0}, {0,0,1,0,0}, {1,0,1,0,1}, {1,0,1,1,0}, {0,1,0,0,1}},
  // ' (39)
  {{0,0,1,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // ( (40)
  {{0,0,0,1,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,0,1,0}},
  // ) (41)
  {{0,1,0,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,1,0,0,0}},
  // * (42)
  {{0,1,0,1,0}, {0,0,1,0,0}, {0,1,1,1,0}, {0,0,1,0,0}, {0,1,0,1,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // + (43)
  {{0,0,0,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,1,1,1,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,0,0,0}},
  // , (44)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,1,0,0,0}, {1,0,0,0,0}},
  // - (45)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // . (46)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // / (47)
  {{0,0,0,0,1}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {1,0,0,0,0}},
  // 0 (48) - 9 (57) sind wie oben, aber hier als Teil des globalen Fonts
  // '0'
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // '1'
  {{0,0,1,0,0}, {0,1,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,1,1,1,0}},
  // '2'
  {{0,1,1,1,0}, {1,0,0,0,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {1,1,1,1,1}},
  // '3'
  {{0,1,1,1,0}, {1,0,0,0,1}, {0,0,0,0,1}, {0,0,1,1,0}, {0,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // '4'
  {{0,0,0,1,0}, {0,0,1,1,0}, {0,1,0,1,0}, {1,0,0,1,0}, {1,1,1,1,1}, {0,0,0,1,0}, {0,0,0,1,0}},
  // '5'
  {{1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,0}, {0,0,0,0,1}, {0,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // '6'
  {{0,0,1,1,0}, {0,1,0,0,0}, {1,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // '7'
  {{1,1,1,1,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}},
  // '8'
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // '9'
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,1,1,0,0}},
  // : (58)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // ; (59)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,1,0,0,0}, {1,0,0,0,0}},
  // < (60)
  {{0,0,0,0,0}, {0,0,1,0,0}, {0,1,0,0,0}, {1,0,0,0,0}, {0,1,0,0,0}, {0,0,1,0,0}, {0,0,0,0,0}},
  // = (61)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,1,1,1,1}, {0,0,0,0,0}, {1,1,1,1,1}, {0,0,0,0,0}, {0,0,0,0,0}},
  // > (62)
  {{0,0,0,0,0}, {0,1,0,0,0}, {0,0,1,0,0}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {0,0,0,0,0}},
  // ? (63)
  {{0,1,1,1,0}, {1,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,0,0,0}, {0,0,1,0,0}},
  // @ (64)
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,1,0,1,1}, {1,0,1,0,1}, {1,0,1,1,1}, {1,0,0,0,0}, {0,1,1,1,0}},
  // A (65)
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // B (66)
  {{1,1,1,1,0}, {1,0,0,0,1}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,0}},
  // C (67)
  {{0,1,1,1,1}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {0,1,1,1,1}},
  // D (68)
  {{1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,0}},
  // E (69)
  {{1,1,1,1,1}, {1,0,0,0,0}, {1,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,1,1,1,1}},
  // F (70)
  {{1,1,1,1,1}, {1,0,0,0,0}, {1,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}},
  // G (71)
  {{0,1,1,1,1}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,1,1,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // H (72)
  {{1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // I (73)
  {{1,1,1,1,1}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {1,1,1,1,1}},
  // J (74)
  {{0,0,0,0,1}, {0,0,0,0,1}, {0,0,0,0,1}, {0,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // K (75)
  {{1,0,0,0,1}, {1,0,0,1,0}, {1,0,1,0,0}, {1,1,0,0,0}, {1,0,1,0,0}, {1,0,0,1,0}, {1,0,0,0,1}},
  // L (76)
  {{1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,1,1,1,1}},
  // M (77)
  {{1,0,0,0,1}, {1,1,0,1,1}, {1,0,1,0,1}, {1,0,1,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // N (78)
  {{1,0,0,0,1}, {1,1,0,0,1}, {1,0,1,0,1}, {1,0,1,0,1}, {1,0,0,1,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // O (79)
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // P (80)
  {{1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,0}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}},
  // Q (81)
  {{0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,1,0,1}, {1,0,0,1,0}, {0,1,1,0,1}},
  // R (82)
  {{1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,0}, {1,0,0,1,0}, {1,0,0,0,1}, {1,0,0,0,1}},
  // S (83)
  {{0,1,1,1,1}, {1,0,0,0,0}, {1,0,0,0,0}, {0,1,1,1,0}, {0,0,0,0,1}, {0,0,0,0,1}, {1,1,1,1,0}},
  // T (84)
  {{1,1,1,1,1}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}},
  // U (85)
  {{1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // V (86)
  {{1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}},
  // W (87)
  {{1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,1,0,1}, {1,0,1,0,1}, {1,1,0,1,1}, {0,1,1,1,0}},
  // X (88)
  {{1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}, {0,1,0,1,0}, {1,0,0,0,1}, {0,0,0,0,0}, {0,0,0,0,0}},
  // Y (89)
  {{1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}},
  // Z (90)
  {{1,1,1,1,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {1,0,0,0,0}, {1,1,1,1,1}},
  // [ (91)
  {{0,1,1,1,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,1,1,0}},
  // \ (92)
  {{1,0,0,0,0}, {0,1,0,0,0}, {0,0,1,0,0}, {0,0,0,1,0}, {0,0,0,0,1}, {0,0,0,0,0}, {0,0,0,0,0}},
  // ] (93)
  {{0,1,1,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,1,1,1,0}},
  // ^ (94)
  {{0,0,1,0,0}, {0,1,0,1,0}, {1,0,0,0,1}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // _ (95)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {1,1,1,1,1}},
  // ` (96)
  {{0,0,1,0,0}, {0,1,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // a (97)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {0,0,0,0,1}, {0,1,1,1,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // b (98)
  {{1,0,0,0,0}, {1,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,0}},
  // c (99)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,1}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}, {0,1,1,1,1}},
  // d (100)
  {{0,0,0,0,1}, {0,0,0,0,1}, {0,1,1,1,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // e (101)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {1,0,0,0,1}, {1,1,1,1,0}, {1,0,0,0,0}, {0,1,1,1,0}},
  // f (102)
  {{0,0,1,1,0}, {0,1,0,0,0}, {0,1,1,1,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}},
  // g (103)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {1,0,0,0,1}, {0,1,1,1,1}, {0,0,0,0,1}, {0,1,1,1,0}},
  // h (104)
  {{1,0,0,0,0}, {1,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // i (105)
  {{0,0,1,0,0}, {0,0,0,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}},
  // j (106)
  {{0,0,0,1,0}, {0,0,0,0,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,1,1,0,0}},
  // k (107)
  {{1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,1,0}, {1,0,1,0,0}, {1,1,0,0,0}, {1,0,1,0,0}, {1,0,0,1,0}},
  // l (108)
  {{0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}},
  // m (109)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,1,0,1,1}, {1,0,1,0,1}, {1,0,1,0,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // n (110)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}},
  // o (111)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // p (112)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,0}, {1,0,0,0,0}},
  // q (113)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,1}, {0,0,0,0,1}},
  // r (114)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,0}, {1,0,0,0,0}, {1,0,0,0,0}},
  // s (115)
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,1,1,1,0}, {1,0,0,0,0}, {0,1,1,1,0}, {0,0,0,0,1}, {1,1,1,1,0}},
  // t (116)
  {{0,0,1,0,0}, {1,1,1,1,1}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,1,0}},
  // u (117)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}},
  // v (118)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,0,1,0}, {0,1,0,1,0}, {0,0,1,0,0}},
  // w (119)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,0,0,0,1}, {1,0,1,0,1}, {1,0,1,0,1}, {1,1,0,1,1}, {0,1,1,1,0}},
  // x (120)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,0,0,0,1}, {0,1,0,1,0}, {0,0,1,0,0}, {0,1,0,1,0}, {1,0,0,0,1}},
  // y (121)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0}, {0,0,0,0,1}, {0,1,1,1,0}},
  // z (122)
  {{0,0,0,0,0}, {0,0,0,0,0}, {1,1,1,1,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {1,1,1,1,1}},
  // { (123)
  {{0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,1,0,0,0}, {0,0,1,0,0}, {0,0,0,1,0}},
  // | (124)
  {{0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}, {0,0,1,0,0}},
  // } (125)
  {{0,0,1,0,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,0,1,0}, {0,0,1,0,0}, {0,0,0,1,0}},
  // ~ (126)
  {{0,0,0,0,0}, {0,1,1,0,0}, {1,0,0,1,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}},
  // DEL (127) - placeholder for the last of 96 chars, typically not drawn
  {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}}
};

// --- Funktions-Prototypen ---
// Dies ist wichtig! Alle Funktionen, die später definiert werden,
// aber schon früher im Code aufgerufen werden, müssen hier deklariert werden.
void setupWiFi();
void handleRoot();
void handleSetTime(); // Diesen Handler gibt es im HTML nicht mehr, aber lassen wir ihn der Vollständigkeit halber
void handleSetMode();
void handleSetColor();
void handleSetBrightness();
void handleSetClockColor();
void handleSetScrollingTextColor();
void handleSetScrollingText();

void displayTime();
void displayScrollingText();
void drawChar(char c, int globalX, CRGB color);
void clearAllLeds();
void rainbowCycle();
void displaySolidColor();
int XY(int x, int y); // Hilfsfunktion für die LED-Mapping

unsigned long previousMillis = 0;
const long interval = 1000; // LEDs jede Sekunde aktualisieren

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, TOTAL_LEDS);
  FastLED.setBrightness(currentBrightness);
  FastLED.clear();
  FastLED.show();

  setupWiFi(); // WLAN Verbindung und AP starten

  // Webserver Routen definieren
  server.on("/", HTTP_GET, handleRoot);
  server.on("/setMode", HTTP_POST, handleSetMode);
  server.on("/setColor", HTTP_POST, handleSetColor);
  server.on("/setBrightness", HTTP_POST, handleSetBrightness);
  server.on("/setClockColor", HTTP_POST, handleSetClockColor);
  server.on("/setScrollingTextColor", HTTP_POST, handleSetScrollingTextColor);
  server.on("/setScrollingText", HTTP_POST, handleSetScrollingText);

  server.begin();
  Serial.println("ESP32 LED Uhr bereit!");
}

void loop() {
  server.handleClient(); // Webserver Anfragen verarbeiten

  // Wenn mit Heim-WLAN verbunden, NTP aktualisieren
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }

  unsigned long currentMillis = millis();

  // Zeitaktualisierung oder Scrolling-Text-Bewegung
  if (currentMode == CLOCK) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (WiFi.status() == WL_CONNECTED) { // Nur aktualisieren, wenn NTP läuft
        setTime(timeClient.getEpochTime());
      }
      displayTime();
    }
  } else if (currentMode == SCROLLING_TEXT) {
    if (currentMillis - lastScrollTime >= scrollSpeed) {
      lastScrollTime = currentMillis;
      displayScrollingText();
    }
  }

  // Modi aktualisieren
  switch (currentMode) {
    case CLOCK:
      // Wird im If-Block oben aktualisiert
      break;
    case RAINBOW_CYCLE:
      rainbowCycle();
      break;
    case SOLID_COLOR:
      displaySolidColor();
      break;
    case SCROLLING_TEXT:
      // Wird im If-Block oben aktualisiert
      break;
  }

  FastLED.show();
  delay(10);
}

// --- WLAN Setup Funktion ---
void setupWiFi() {
  // Startet den ESP32 als Station (Client für dein Heim-WLAN)
  WiFi.mode(WIFI_AP_STA); // Setzt den Modus auf Access Point UND Station
  delay(100); // Kurze Pause

  // Konfiguriere den Access Point (AP)
  Serial.print("Starte AP (Access Point): ");
  Serial.println(AP_SSID);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP Adresse: ");
  Serial.println(apIP);
  Serial.println("Verbinde dich mit diesem WLAN und gehe im Browser zu 192.168.4.1");

  // Versuche, dich mit dem Heim-WLAN zu verbinden (Station Mode)
  Serial.print("Versuche, mich mit Heim-WLAN zu verbinden: ");
  Serial.println(STA_SSID);
  WiFi.begin(STA_SSID, STA_PASSWORD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 20) { // Warte bis zu 10 Sekunden (20 * 500ms)
    delay(500);
    Serial.print(".");
    counter++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nVerbunden mit Heim-WLAN!");
    Serial.print("Client IP Adresse: ");
    Serial.println(WiFi.localIP());
    timeClient.begin(); // NTP nur starten, wenn mit Heim-WLAN verbunden
    timeClient.update();
    setTime(timeClient.getEpochTime());
    currentMode = CLOCK; // Standardmäßig Uhr anzeigen nach erfolgreicher Verbindung
  } else {
    Serial.println("\nVerbindung mit Heim-WLAN fehlgeschlagen oder keine Verbindung möglich.");
    Serial.println("Der ESP32 betreibt weiterhin seinen eigenen AP.");
    currentMode = SOLID_COLOR; // Zeigt an, dass keine Uhrzeit aus dem Netz kommt
    solidColor = CRGB::Red; // Rot für "kein WLAN"
    FastLED.setBrightness(50); // Setze Helligkeit sichtbar für rote Farbe
    displaySolidColor();
    FastLED.show();
  }
}

// --- Webseiten HTML (Haupt-Steuerung) ---
void handleRoot() {
  char solidColorHex[7];
  sprintf(solidColorHex, "%02X%02X%02X", solidColor.r, solidColor.g, solidColor.b);

  char clockColorHex[7];
  sprintf(clockColorHex, "%02X%02X%02X", clockDigitColor.r, clockDigitColor.g, clockDigitColor.b);

  char scrollingTextColorHex[7];
  sprintf(scrollingTextColorHex, "%02X%02X%02X", scrollingTextColor.r, scrollingTextColor.g, scrollingTextColor.b);

  String wifiStatusText;
  if (WiFi.status() == WL_CONNECTED) {
    wifiStatusText = "Verbunden mit: " + String(STA_SSID) + " (IP: " + WiFi.localIP().toString() + ")";
  } else {
    wifiStatusText = "Nicht mit Heim-WLAN verbunden. Nur AP aktiv.";
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 LED Uhr Steuerung</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #2c3e50; color: #ecf0f1; margin: 0; padding: 20px; }
        .container { max-width: 800px; margin: auto; background-color: #34495e; padding: 30px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.3); }
        h1, h2 { color: #3498db; text-align: center; }
        .section { margin-bottom: 25px; padding: 20px; background-color: #2c3e50; border-radius: 6px; border: 1px solid #3498db; }
        label { display: block; margin-bottom: 8px; font-weight: bold; color: #ecf0f1; }
        input[type="range"], input[type="color"], select, button, input[type="text"] {
            width: calc(100% - 20px);
            padding: 12px;
            margin-bottom: 15px;
            border-radius: 5px;
            border: 1px solid #3498db;
            background-color: #ecf0f1;
            color: #2c3e50;
            font-size: 16px;
        }
        button {
            background-color: #27ae60;
            color: white;
            cursor: pointer;
            transition: background-color 0.3s ease;
            width: 100%;
        }
        button:hover { background-color: #2ecc71; }
        .mode-buttons button {
            background-color: #9b59b6;
            margin-right: 10px;
            width: auto;
            padding: 12px 25px;
        }
        .mode-buttons button:hover { background-color: #8e44ad; }
        .color-preview {
            width: 100%;
            height: 50px;
            border: 1px solid #ccc;
            margin-top: 10px;
            border-radius: 5px;
        }
        footer { text-align: center; margin-top: 30px; font-size: 0.9em; color: #bdc3c7; }
        .slider-value { text-align: right; margin-top: -10px; margin-bottom: 10px; font-weight: bold; }
        .wifi-status {
            background-color: #f1c40f;
            color: #2c3e50;
            padding: 10px;
            border-radius: 5px;
            text-align: center;
            margin-bottom: 20px;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>✨ ESP32 LED Uhr & Lichtsteuerung ✨</h1>
        <p style="text-align: center; color: #f1c40f;">Willkommen zu deiner kunterbunten LED-Welt!</p>

        <div class="wifi-status">
            WLAN Status: %WIFI_STATUS_TEXT%
        </div>

        <div class="section">
            <h2>💡 Helligkeit einstellen</h2>
            <label for="brightnessSlider">Globale Helligkeit (0-255):</label>
            <input type="range" id="brightnessSlider" min="0" max="255" value="%CURRENT_BRIGHTNESS%" oninput="updateBrightnessValue(this.value)">
            <div class="slider-value" id="brightnessValue">%CURRENT_BRIGHTNESS%</div>
            <button onclick="setBrightness()">Helligkeit anwenden</button>
        </div>

        <div class="section">
            <h2>🌈 Anzeigemodus</h2>
            <div class="mode-buttons">
                <button onclick="setMode('CLOCK')">Uhr</button>
                <button onclick="setMode('RAINBOW_CYCLE')">Regenbogen</button>
                <button onclick="setMode('SOLID_COLOR')">Feste Farbe</button>
                <button onclick="setMode('SCROLLING_TEXT')">Lauftext</button>
            </div>
            <p>Aktueller Modus: <strong id="currentModeDisplay">Uhr</strong></p>
        </div>

        <div class="section">
            <h2>🎨 Farben anpassen</h2>
            <p>Hier kannst du die Farben für die verschiedenen Anzeigemodi einstellen:</p>

            <label for="clockColorPicker">Farbe der Uhrziffern (im Modus "Uhr"):</label>
            <input type="color" id="clockColorPicker" value="#%CLOCK_COLOR_HEX%">
            <div class="color-preview" id="clockColorPreview" style="background-color: #%CLOCK_COLOR_HEX%;"></div>
            <button onclick="setClockColor()">Uhrziffernfarbe anwenden</button>

            <br><br>

            <label for="colorPicker">Farbe für den Modus "Feste Farbe":</label>
            <input type="color" id="colorPicker" value="#%SOLID_COLOR_HEX%">
            <div class="color-preview" id="colorPreview" style="background-color: #%SOLID_COLOR_HEX%;"></div>
            <button onclick="setSolidColor()">Feste Farbe anwenden</button>

            <br><br>

            <label for="scrollingTextColorPicker">Farbe für den Lauftext (im Modus "Lauftext"):</label>
            <input type="color" id="scrollingTextColorPicker" value="#%SCROLLING_TEXT_COLOR_HEX%">
            <div class="color-preview" id="scrollingTextColorPreview" style="background-color: #%SCROLLING_TEXT_COLOR_HEX%;"></div>
            <button onclick="setScrollingTextColor()">Lauftextfarbe anwenden</button>
        </div>

        <div class="section">
            <h2>📝 Lauftext einstellen</h2>
            <label for="scrollingTextInput">Gib deinen Text ein:</label>
            <input type="text" id="scrollingTextInput" value="%SCROLLING_TEXT%">
            <button onclick="setScrollingText()">Lauftext starten</button>
            <p style="font-size:0.9em; color:#bdc3c7;">Der Text läuft dann von rechts nach links durch.</p>
        </div>
    </div>

    <footer>
        <p>&copy; 2025 Dein LED-Projekt</p>
    </footer>

    <script>
        // Mapping von Enum-Werten zu lesbaren Texten für die Anzeige
        const modeMap = {
            'CLOCK': 'Uhr',
            'RAINBOW_CYCLE': 'Regenbogen',
            'SOLID_COLOR': 'Feste Farbe',
            'SCROLLING_TEXT': 'Lauftext'
        };

        // Initialer Wert, wird beim Laden der Seite von ESP32 gesetzt
        let currentDisplayMode = '%CURRENT_MODE_ENUM%'; // Placeholder wird vom ESP32 ersetzt

        document.addEventListener('DOMContentLoaded', (event) => {
            document.getElementById('colorPicker').addEventListener('input', function() {
                document.getElementById('colorPreview').style.backgroundColor = this.value;
            });
            document.getElementById('clockColorPicker').addEventListener('input', function() {
                document.getElementById('clockColorPreview').style.backgroundColor = this.value;
            });
            document.getElementById('scrollingTextColorPicker').addEventListener('input', function() {
                document.getElementById('scrollingTextColorPreview').style.backgroundColor = this.value;
            });
            updateBrightnessValue(document.getElementById('brightnessSlider').value);
            updateCurrentModeDisplay();
        });

        function updateBrightnessValue(val) {
            document.getElementById('brightnessValue').textContent = val;
        }

        async function setBrightness() {
            const brightness = document.getElementById('brightnessSlider').value;
            const response = await fetch('/setBrightness', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `brightness=${brightness}`
            });
            if (response.ok) {
                //alert(`Helligkeit auf ${brightness} eingestellt!`);
            } else {
                alert("Fehler beim Einstellen der Helligkeit.");
            }
        }

        async function setMode(mode) {
            const response = await fetch('/setMode', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `mode=${mode}`
            });
            if (response.ok) {
                currentDisplayMode = mode; // Update client-side state
                updateCurrentModeDisplay();
            } else {
                alert("Fehler beim Einstellen des Modus.");
            }
        }

        async function setSolidColor() {
            const colorInput = document.getElementById('colorPicker').value;
            const hexColor = colorInput.substring(1);
            const response = await fetch('/setColor', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `color=${hexColor}`
            });
            if (response.ok) {
                currentDisplayMode = 'SOLID_COLOR';
                updateCurrentModeDisplay();
            } else {
                alert("Fehler beim Einstellen der Farbe.");
            }
        }

        async function setClockColor() {
            const colorInput = document.getElementById('clockColorPicker').value;
            const hexColor = colorInput.substring(1);
            const response = await fetch('/setClockColor', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `color=${hexColor}`
            });
            if (response.ok) {
                //alert("Uhrziffernfarbe eingestellt!");
            } else {
                alert("Fehler beim Einstellen der Ziffernfarbe.");
            }
        }

        async function setScrollingTextColor() {
            const colorInput = document.getElementById('scrollingTextColorPicker').value;
            const hexColor = colorInput.substring(1);
            const response = await fetch('/setScrollingTextColor', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `color=${hexColor}`
            });
            if (response.ok) {
                //alert("Lauftextfarbe eingestellt!");
            } else {
                alert("Fehler beim Einstellen der Lauftextfarbe.");
            }
        }

        async function setScrollingText() {
            const textInput = document.getElementById('scrollingTextInput').value;
            const response = await fetch('/setScrollingText', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `text=${encodeURIComponent(textInput)}`
            });
            if (response.ok) {
                currentDisplayMode = 'SCROLLING_TEXT';
                updateCurrentModeDisplay();
            } else {
                alert("Fehler beim Starten des Lauftextes.");
            }
        }

        function updateCurrentModeDisplay() {
            document.getElementById('currentModeDisplay').textContent = modeMap[currentDisplayMode];
        }
    </script>
</body>
</html>
)rawliteral";

  // Konvertiere Enum zu String für HTML-Platzhalter
  String currentModeString;
  switch(currentMode) {
    case CLOCK: currentModeString = "CLOCK"; break;
    case RAINBOW_CYCLE: currentModeString = "RAINBOW_CYCLE"; break;
    case SOLID_COLOR: currentModeString = "SOLID_COLOR"; break;
    case SCROLLING_TEXT: currentModeString = "SCROLLING_TEXT"; break;
  }

  String finalHtml = html;
  finalHtml.replace("%WIFI_STATUS_TEXT%", wifiStatusText);
  finalHtml.replace("%CURRENT_BRIGHTNESS%", String(currentBrightness));
  finalHtml.replace("%SOLID_COLOR_HEX%", solidColorHex);
  finalHtml.replace("%CLOCK_COLOR_HEX%", clockColorHex);
  finalHtml.replace("%SCROLLING_TEXT_COLOR_HEX%", scrollingTextColorHex);
  finalHtml.replace("%SCROLLING_TEXT%", scrollingText);
  finalHtml.replace("%CURRENT_MODE_ENUM%", currentModeString);

  server.send(200, "text/html", finalHtml);
}


// --- Webserver Request Handler ---
void handleSetTime() {
  currentMode = CLOCK;
  displayTime(); // Direkt aktualisieren
  server.send(200, "text/plain", "Uhrzeit-Modus aktiviert (Synchronisation über Internet)");
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    String modeStr = server.arg("mode");
    if (modeStr == "CLOCK") {
      currentMode = CLOCK;
      displayTime(); // Direkt aktualisieren
    } else if (modeStr == "RAINBOW_CYCLE") {
      currentMode = RAINBOW_CYCLE;
      clearAllLeds();
    } else if (modeStr == "SOLID_COLOR") {
      currentMode = SOLID_COLOR;
      clearAllLeds();
    } else if (modeStr == "SCROLLING_TEXT") {
      currentMode = SCROLLING_TEXT;
      clearAllLeds();
      scrollOffset = 0; // Reset scroll position when entering mode
      displayScrollingText(); // Initial draw
    }
    server.send(200, "text/plain", "Modus aktualisiert");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSetColor() {
  if (server.hasArg("color")) {
    String colorHex = server.arg("color");
    long number = (long) strtol(colorHex.c_str(), NULL, 16);
    int r = (number >> 16) & 0xFF;
    int g = (number >> 8) & 0xFF;
    int b = number & 0xFF;
    solidColor = CRGB(r, g, b);
    currentMode = SOLID_COLOR; // Automatisch in diesen Modus wechseln
    server.send(200, "text/plain", "Feste Farbe aktualisiert");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSetBrightness() {
  if (server.hasArg("brightness")) {
    int brightness = server.arg("brightness").toInt();
    if (brightness >= 0 && brightness <= 255) {
      currentBrightness = (uint8_t)brightness;
      FastLED.setBrightness(currentBrightness);
      server.send(200, "text/plain", "Helligkeit aktualisiert");
    } else {
      server.send(400, "text/plain", "Ungültiger Helligkeitswert");
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSetClockColor() {
  if (server.hasArg("color")) {
    String colorHex = server.arg("color");
    long number = (long) strtol(colorHex.c_str(), NULL, 16);
    int r = (number >> 16) & 0xFF;
    int g = (number >> 8) & 0xFF;
    int b = number & 0xFF;
    clockDigitColor = CRGB(r, g, b);
    server.send(200, "text/plain", "Uhrziffernfarbe aktualisiert");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSetScrollingTextColor() {
  if (server.hasArg("color")) {
    String colorHex = server.arg("color");
    long number = (long) strtol(colorHex.c_str(), NULL, 16);
    int r = (number >> 16) & 0xFF;
    int g = (number >> 8) & 0xFF;
    int b = number & 0xFF;
    scrollingTextColor = CRGB(r, g, b);
    server.send(200, "text/plain", "Lauftextfarbe aktualisiert");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSetScrollingText() {
  if (server.hasArg("text")) {
    scrollingText = server.arg("text");
    scrollingText.replace("+", " "); // Handle spaces from URL encoding
    currentMode = SCROLLING_TEXT; // Automatisch in diesen Modus wechseln
    scrollOffset = 0; // Reset scroll position when new text is set
    displayScrollingText(); // Initial draw
    server.send(200, "text/plain", "Lauftext aktualisiert");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// --- LED Anzeige Funktionen ---

// Mapping-Funktion für 8x8 Panel (Standard, Zeilenweise, ungerade Zeilen gespiegelt)
// Hinweis: Überprüfe, ob deine Panels "Serpentine" (Schlangenlinien-Anordnung) sind.
// Wenn die Zeilenrichtung wechselt (z.B. Zeile 0 von links nach rechts, Zeile 1 von rechts nach links),
// dann müsste die XY-Funktion angepasst werden. Die aktuelle ist für einfache, gerade Zeilen.
int XY(int x, int y) {
  if (x < 0 || x >= 8 || y < 0 || y >= 8) return -1; // Außerhalb der Grenzen

  // Standardmäßige Zeilenordnung (nicht Serpentine)
  // Wenn deine Matrizen Serpentine sind, musst du das anpassen:
  // if (y % 2 == 1) { // Ungerade Zeilen sind gespiegelt
  //   return (y * 8) + (7 - x);
  // } else { // Gerade Zeilen sind normal
  //   return (y * 8) + x;
  // }
  return (y * 8) + x; // Einfachste Form: alle Zeilen gehen in die gleiche Richtung
}

void clearAllLeds() {
  FastLED.clear();
}

void drawChar(char c, int globalX, CRGB color) {
  // Stellen Sie sicher, dass das Zeichen im gültigen ASCII-Bereich liegt
  if (c < 32 || c > 127) {
    c = 32; // Standardmäßig Leerzeichen verwenden, wenn außerhalb des Bereichs
  }

  // Das Font-Array ist von ASCII 32 (' ') bis 127 ('DEL') indiziert.
  // Wir müssen den Zeichen-Index um 32 verschieben, um auf das Array zuzugreifen.
  int charIndex = c - 32;

  for (int y = 0; y < 7; y++) { // 7 Zeilen (Höhe des Zeichens)
    for (int x = 0; x < 5; x++) { // 5 Spalten (Breite des Zeichens)
      if (ASCII_FONT[charIndex][y][x] == 1) {
        int panelIndex = (globalX + x) / 8;
        int xInPanel = (globalX + x) % 8;
        int ledIndex = (panelIndex * NUM_LEDS_PER_PANEL) + XY(xInPanel, y);
        if (ledIndex != -1 && ledIndex < TOTAL_LEDS) {
          leds[ledIndex] = color;
        }
      }
    }
  }
}

void displayScrollingText() {
  clearAllLeds(); // Zuerst alles löschen

  // Um einen flüssigen Übergang zu haben, zeichnen wir den aktuellen Text
  // und einen Teil des Textes, der von rechts nachkommt.
  String fullText = scrollingText + "   "; // Zusätzlicher Abstand am Ende
  int totalTextWidth = fullText.length() * 6; // Jedes Zeichen ist 5 breit + 1 Spalte Abstand

  // Der sichtbare Bereich geht von 0 bis TOTAL_LEDS (24 LEDs).
  // Wir müssen den Text so zeichnen, dass er sich von rechts nach links bewegt.

  for (int i = 0; i < fullText.length(); i++) {
    char currentChar = fullText.charAt(i);
    int charWidth = 5; // Breite eines Zeichens

    // Berechne die Startposition des aktuellen Zeichens auf dem gesamten Display
    // scrollOffset verschiebt den Text nach links (negativ)
    int charGlobalX = (i * (charWidth + 1)) + scrollOffset;

    // Zeichne das Zeichen, wenn es (teilweise) sichtbar ist
    // Es ist sichtbar, wenn sein rechter Rand >= 0 und sein linker Rand < Gesamtbreite ist
    if (charGlobalX + charWidth >= 0 && charGlobalX < (NUM_PANELS * 8)) {
      drawChar(currentChar, charGlobalX, scrollingTextColor);
    }
  }

  // Lauftext-Offset aktualisieren: Text bewegt sich nach links
  scrollOffset--;

  // Wenn der Text komplett vom Bildschirm verschwunden ist, setze den Offset zurück
  // Der Text ist "weg", wenn der Anfang des Textes (globalX = 0) außerhalb des Displays ist
  // Das ist der Fall, wenn scrollOffset + totalTextWidth <= 0 ist.
  // Wir wollen aber, dass er wieder von rechts erscheint, wenn der letzte Buchstabe verschwindet.
  if (scrollOffset < -totalTextWidth) {
    scrollOffset = (NUM_PANELS * 8); // Startet wieder ganz rechts außerhalb des Displays
  }
}

// Uhrzeit auf den 3 Panelen anzeigen (HH:MM)
void displayTime() {
  clearAllLeds();

  int h = hour();
  int m = minute();

  int digit1 = h / 10;
  int digit2 = h % 10;
  int digit3 = m / 10;
  int digit4 = m % 10;

  // Zeichenpositionen angepasst für 3 Panels (24 Pixel Breite)
  // Panel 1: HH (0-7)
  // Panel 2: :M (8-15)
  // Panel 3: M (16-23)
  // Breite pro Ziffer ist 5 Pixel + 1 Pixel Abstand = 6 Pixel
  // H: 0, H: 6, : 12, M: 14, M: 20
  drawChar(String(digit1).charAt(0), 0, clockDigitColor);    // Erste Stunde
  drawChar(String(digit2).charAt(0), 6, clockDigitColor);    // Zweite Stunde

  // Doppelpunkt in der Mitte des zweiten Panels, zwischen den Stunden und Minuten
  int globalXForDots = 12; // Startposition für den Doppelpunkt
  int dotY1 = 2; // Y-Position des oberen Punktes
  int dotY2 = 5; // Y-Position des unteren Punktes

  // Berechnung der LED-Indizes für die Punkte
  int panelIndexDot1 = globalXForDots / 8;
  int xInPanelDot1 = globalXForDots % 8;
  int ledIndexDot1 = (panelIndexDot1 * NUM_LEDS_PER_PANEL) + XY(xInPanelDot1, dotY1);
  if (ledIndexDot1 != -1 && ledIndexDot1 < TOTAL_LEDS) {
    leds[ledIndexDot1] = CRGB::White; // Oder eine andere Farbe für den Doppelpunkt
  }

  int panelIndexDot2 = globalXForDots / 8;
  int xInPanelDot2 = globalXForDots % 8;
  int ledIndexDot2 = (panelIndexDot2 * NUM_LEDS_PER_PANEL) + XY(xInPanelDot2, dotY2);
  if (ledIndexDot2 != -1 && ledIndexDot2 < TOTAL_LEDS) {
    leds[ledIndexDot2] = CRGB::White; // Oder eine andere Farbe für den Doppelpunkt
  }

  drawChar(String(digit3).charAt(0), 14, clockDigitColor);   // Erste Minute
  drawChar(String(digit4).charAt(0), 20, clockDigitColor);   // Zweite Minute
}

void rainbowCycle() {
  fill_rainbow(leds, TOTAL_LEDS, rainbowHue, 7);
  rainbowHue++; // Hue rotieren
  if (rainbowHue >= 255) rainbowHue = 0;
  delay(20); // Schneller, flüssiger Regenbogen
}

void displaySolidColor() {
  fill_solid(leds, TOTAL_LEDS, solidColor);
}