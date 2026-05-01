/*
 * ╔═════════════════════════════════════════╗
 * ║  RED DEVICE — ClawMagotchi v0.2         ║
 * ║  Clock + Lobster + WiFi + Notifications ║
 * ║  ST7789 (real) / ILI9341 (Wokwi)       ║
 * ╚═════════════════════════════════════════╝
 *
 * Idle screen: a digital clock with an animated
 * lobster companion roaming the bottom of the
 * display. Connects to WiFi, syncs time via NTP,
 * and polls Clawpilot for live notifications.
 *
 * Hardware:  Waveshare ESP32-C6 LCD 1.47" (ST7789)
 * Emulator:  Wokwi with ILI9341 (240×320)
 */

#include "config.h"
#include <SPI.h>
#include <Adafruit_GFX.h>

#ifdef PLATFORM_WAVESHARE
  #include <Adafruit_ST7789.h>
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>
  #include <time.h>
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#else
  #include <Adafruit_ILI9341.h>
  Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
#endif

#include "segments.h"
#include "sprites.h"
#include "scenarios.h"

// ────────────────────────────────────────────
//  RGB565 COLOR PALETTE
// ────────────────────────────────────────────
#define C_BG       0x18C5   // dark navy  #1A1A2E
#define C_WHITE    0xFFFF
#define C_DIMWH    0x31A6   // dim colon
#define C_GRAY     0x7BEF
#define C_LGRAY    0xAD75
#define C_RED      0xE1C8   // lobster red #E63946
#define C_DRED     0xC083   // shadow red  #C1121F
#define C_GOLD     0xFE8C   // claw gold   #FFD166
#define C_BLACK    0x0000
#define C_GROUND   0x2104   // subtle ground
#define C_STATUS   0x04A0   // teal status dot
#define C_WARN     0xFD20   // orange for urgent
#define C_NOTIF    0x2C9F   // blue for notifications
#define C_TEAMS    0x541F   // purple for Teams

// ════════════════════════════════════════════
//  CLOCK STATE
// ════════════════════════════════════════════
unsigned long tBase;
long simSec;
int prevH = -1, prevM = -1, prevS = -1;
bool colonVis = true;
bool timeFromNTP = false;

// ════════════════════════════════════════════
//  WIFI STATE
// ════════════════════════════════════════════
bool wifiConnected = false;
unsigned long lastPoll = 0;
unsigned long lastWifiCheck = 0;

// ════════════════════════════════════════════
//  NOTIFICATION STATE
// ════════════════════════════════════════════
#define MAX_NOTIFS 5
struct Notification {
  char type[16];     // "email", "teams", "calendar", "urgent"
  char title[64];
  char body[128];
  char timeStr[8];   // "09:41"
  bool active;
};
Notification notifs[MAX_NOTIFS];
int notifCount = 0;
bool showingNotif = false;
unsigned long notifShowTime = 0;
int currentNotif = 0;

// ════════════════════════════════════════════
//  SCENARIO STATE
// ════════════════════════════════════════════
Scenario currentScenario = SC_IDLE;

// ════════════════════════════════════════════
//  LOBSTER STATE MACHINE
// ════════════════════════════════════════════
enum LState { SIT, GO_R, GO_L, LOOK };
#define _LMOOD_DEFINED
enum LMood  { IDLE, HAPPY, ALERT, URGENT };

struct {
  float x;
  int y;
  LState st;
  LMood mood;
  int fr;
  int dir;
  unsigned long nextSt;
  unsigned long nextFr;
} L;

// Bounding box of previous frame (for erase)
int pL = 0, pR = 0, pT = 0, pB = 0;

// Ground Y position
#define GROUND_Y (SH - 34)
#define LOB_Y    (SH - 18)

// ════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0) ^ micros());

  // Backlight on (real hardware)
  #ifdef PLATFORM_WAVESHARE
    if (TFT_BL >= 0) {
      pinMode(TFT_BL, OUTPUT);
      analogWrite(TFT_BL, 200);  // ~80% brightness
    }
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
    tft.init(172, 320);
  #else
    tft.begin();
  #endif

  tft.setRotation(0);
  tft.fillScreen(C_BG);

  // Dotted ground line
  drawGroundFull();

  // Status: starting
  drawStatus("booting", C_GRAY);

  // Clock (start at 12:00:00 — will be replaced by NTP)
  tBase = millis();
  simSec = 12L * 3600;
  seg7_init();
  seg7_renderTime(12, 0, true);
  seg7_renderSec(0);
  seg7_renderDate("Thu, May 1");

  // Version label
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND);
  tft.setCursor(SW - 60, SH - 10);
  tft.print("Red v0.2");

  // Lobster init
  L.x = SW / 2;
  L.y = LOB_Y;
  L.st = SIT;
  L.mood = IDLE;
  L.fr = 0;
  L.dir = 1;
  L.nextSt = millis() + random(3000, 7000);
  L.nextFr = millis();

  Serial.println("[Red Device] ClawMagotchi v0.2 booting...");

  // WiFi + NTP (real hardware only)
  #ifdef PLATFORM_WAVESHARE
    connectWiFi();
    if (wifiConnected) {
      configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
      Serial.println("[NTP] Time sync requested");
    }
  #endif

  drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);
  Serial.println("[Red Device] Ready!");
}

// ════════════════════════════════════════════
//  MAIN LOOP
// ════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // Clock: tick every 500ms (colon blink)
  static unsigned long lastTick = 0;
  if (now - lastTick >= 500) {
    lastTick = now;
    tickClock(now);
  }

  // Lobster: ~12 fps
  if (now >= L.nextFr) {
    L.nextFr = now + 83;
    tickLobster(now);
  }

  // Notification display timeout (show for 8 sec, then return to clock)
  if (showingNotif && (now - notifShowTime > 8000)) {
    dismissNotification();
  }

  #ifdef PLATFORM_WAVESHARE
    // Poll for notifications
    if (wifiConnected && (now - lastPoll > POLL_INTERVAL)) {
      lastPoll = now;
      pollNotifications();
    }

    // WiFi reconnect check every 30s
    if (!wifiConnected && (now - lastWifiCheck > 30000)) {
      lastWifiCheck = now;
      connectWiFi();
      drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);
    }
  #endif

  // Also accept notifications via Serial (works on both platforms)
  checkSerial();
}

// ════════════════════════════════════════════
//  CLOCK RENDERING
// ════════════════════════════════════════════

void tickClock(unsigned long now) {
  int h, m, s;

  #ifdef PLATFORM_WAVESHARE
    if (timeFromNTP || wifiConnected) {
      struct tm ti;
      if (getLocalTime(&ti, 100)) {
        h = ti.tm_hour;
        m = ti.tm_min;
        s = ti.tm_sec;
        timeFromNTP = true;
      } else {
        // Fallback to simulated time
        long t = (simSec + (now - tBase) / 1000) % 86400;
        h = t / 3600;
        m = (t % 3600) / 60;
        s = t % 60;
      }
    } else
  #endif
  {
    long t = (simSec + (now - tBase) / 1000) % 86400;
    h = t / 3600;
    m = (t % 3600) / 60;
    s = t % 60;
  }

  colonVis = !colonVis;

  if (h != prevH || m != prevM) {
    seg7_renderTime(h, m, colonVis);
    prevH = h;
    prevM = m;
  } else {
    seg7_renderTime(prevH, prevM, colonVis);
  }

  if (s != prevS) {
    seg7_renderSec(s);
    prevS = s;

    // Update date from NTP once per minute
    #ifdef PLATFORM_WAVESHARE
    if (s == 0 && timeFromNTP) {
      struct tm ti;
      if (getLocalTime(&ti, 100)) {
        const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        const char* mons[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        char buf[20];
        snprintf(buf, sizeof(buf), "%s, %s %d", days[ti.tm_wday], mons[ti.tm_mon], ti.tm_mday);
        seg7_renderDate(buf);
      }
    }
    #endif
  }
}

// Adaptive text sizing for narrow screens
#define CLOCK_SIZE  ((SW >= 200) ? 6 : 5)
#define CLOCK_CW    ((SW >= 200) ? 36 : 30)
#define CLOCK_Y     ((SW >= 200) ? 55 : 55)

void renderTime(int h, int m, bool col) {
  const int y = CLOCK_Y;
  const int cw = CLOCK_CW;
  int x0 = (SW - cw * 5) / 2;

  tft.setTextSize(CLOCK_SIZE);
  tft.setTextColor(C_WHITE, C_BG);

  tft.setCursor(x0, y);
  if (h < 10) tft.print('0');
  tft.print(h);

  renderColon(col);

  tft.setCursor(x0 + cw * 3, y);
  if (m < 10) tft.print('0');
  tft.print(m);
}

void renderColon(bool on) {
  const int y = CLOCK_Y;
  const int cw = CLOCK_CW;
  int x0 = (SW - cw * 5) / 2;

  tft.setTextSize(CLOCK_SIZE);
  tft.setTextColor(on ? C_WHITE : C_DIMWH, C_BG);
  tft.setCursor(x0 + cw * 2, y);
  tft.print(':');
}

void renderSec(int s) {
  int x = (SW - 24) / 2;
  tft.setTextSize(2);
  tft.setTextColor(C_LGRAY, C_BG);
  tft.setCursor(x, CLOCK_Y + (CLOCK_SIZE * 8) + 4);
  if (s < 10) tft.print('0');
  tft.print(s);
}

void renderDate() {
  const char* d = "Thu, May 1";
  int w = strlen(d) * 12;
  tft.setTextSize(2);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setCursor((SW - w) / 2, CLOCK_Y + (CLOCK_SIZE * 8) + 28);
  tft.print(d);
}

#ifdef PLATFORM_WAVESHARE
void renderDateNTP() {
  struct tm ti;
  if (!getLocalTime(&ti, 100)) return;
  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  const char* mons[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char buf[20];
  snprintf(buf, sizeof(buf), "%s, %s %d", days[ti.tm_wday], mons[ti.tm_mon], ti.tm_mday);
  int w = strlen(buf) * 12;
  int dateY = CLOCK_Y + (CLOCK_SIZE * 8) + 28;
  tft.fillRect(0, dateY, SW, 18, C_BG);
  tft.setTextSize(2);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setCursor((SW - w) / 2, dateY);
  tft.print(buf);
}
#endif

// ════════════════════════════════════════════
//  GROUND
// ════════════════════════════════════════════

void drawGroundFull() {
  for (int i = 0; i < SW; i += 2) {
    tft.drawPixel(i, GROUND_Y, C_GROUND);
  }
}

void drawGroundRange(int x1, int x2) {
  x1 = max(x1, 0);
  x2 = min(x2, SW);
  for (int i = x1; i < x2; i += 2) {
    tft.drawPixel(i, GROUND_Y, C_GROUND);
  }
}

// ════════════════════════════════════════════
//  LOBSTER 🦞  — State Machine & Animation
// ════════════════════════════════════════════

void tickLobster(unsigned long now) {
  // Erase previous frame
  if (pR > pL) {
    tft.fillRect(pL, pT, pR - pL, pB - pT, C_BG);
    // Restore ground dots inside erased area
    if (pT <= GROUND_Y && pB >= GROUND_Y) {
      drawGroundRange(pL, pR);
    }
  }

  // ── State transitions ──
  if (now >= L.nextSt) {
    int r = random(100);

    if (L.st == LOOK) {
      // After looking around, pick a new action
      if (r < 40)      { L.st = SIT;  L.nextSt = now + random(3000, 7000); }
      else if (r < 70) { L.st = GO_R; L.dir = 1;  L.nextSt = now + random(2000, 5000); }
      else             { L.st = GO_L; L.dir = -1; L.nextSt = now + random(2000, 5000); }
    }
    else if (r < 30) {
      L.st = SIT;
      L.nextSt = now + random(3000, 8000);
    }
    else if (r < 50) {
      L.st = GO_R;
      L.dir = 1;
      L.nextSt = now + random(2000, 5000);
    }
    else if (r < 70) {
      L.st = GO_L;
      L.dir = -1;
      L.nextSt = now + random(2000, 5000);
    }
    else {
      L.st = LOOK;
      L.nextSt = now + random(1500, 3000);
    }
  }

  // ── Movement ──
  float speed = 0.8;
  if (L.st == GO_R) {
    L.x += speed;
    if (L.x > SW - 30) { L.x = SW - 30; L.st = SIT; L.nextSt = now + 2000; }
  }
  else if (L.st == GO_L) {
    L.x -= speed;
    if (L.x < 30) { L.x = 30; L.st = SIT; L.nextSt = now + 2000; }
  }
  else if (L.st == LOOK) {
    if (L.fr % 10 == 0) L.dir = -L.dir;
  }

  L.fr++;

  // ── Draw ──
  drawLob((int)L.x, L.y, L.fr, L.dir);
}

// ────────────────────────────────────────────
//  DRAW THE LOBSTER
//
//  Procedural pixel art using drawing primitives.
//  cx,cy = center of body. d = facing direction.
//  All coordinates computed relative to center.
// ────────────────────────────────────────────

void drawLob(int cx, int cy, int f, int d) {
  // Store bounding box for next erase
  pL = cx - 27;
  pR = cx + 27;
  pT = cy - 23;
  pB = cy + 13;

  // ── TAIL ──
  // Fan (triangle at end)
  int ftx = cx - d * 22;   // fan tip
  int fbx = cx - d * 15;   // fan base
  tft.fillTriangle(fbx, cy - 3, fbx, cy + 4, ftx, cy, C_DRED);
  // Segments
  tft.fillCircle(cx - d * 12, cy, 4, C_RED);
  tft.fillCircle(cx - d * 7,  cy + 1, 5, C_RED);

  // ── BODY ──
  tft.fillRoundRect(cx - 8, cy - 5, 16, 11, 5, C_RED);
  // Belly shadow
  tft.drawFastHLine(cx - 5, cy + 5, 10, C_DRED);

  // ── HEAD ──
  tft.fillCircle(cx + d * 8, cy - 1, 6, C_RED);

  // ── EYE ──
  bool blink = (f % 80 > 75);
  if (!blink) {
    tft.fillCircle(cx + d * 10, cy - 3, 2, C_WHITE);
    tft.fillCircle(cx + d * 11, cy - 3, 1, C_BLACK);
  } else {
    tft.drawFastHLine(cx + d * 9, cy - 3, 3, C_WHITE);
  }

  // ── CLAWS ──
  int co = (f % 16 < 8) ? 0 : 2;  // open/close cycle
  int clawX = cx + d * 18;

  // Arm connecting head to claws
  int ax1 = cx + d * 8;
  int ax2 = cx + d * 14;
  int armL = (d > 0) ? ax1 : ax2;
  int armW = abs(ax2 - ax1) + 1;
  tft.fillRect(armL, cy - 4, armW, 3, C_RED);

  // Upper pincer
  tft.fillCircle(clawX, cy - 5 - co, 3, C_GOLD);
  // Lower pincer
  tft.fillCircle(clawX, cy + 0 + co, 3, C_GOLD);
  // Pincer tips
  tft.drawPixel(clawX + d * 3, cy - 5 - co, C_DRED);
  tft.drawPixel(clawX + d * 3, cy + 0 + co, C_DRED);

  // ── ANTENNAE ──
  int w1 = ((f / 3) % 4 < 2) ? -2 : 2;
  int w2 = ((f / 4) % 3 < 2) ?  2 : -2;
  int a1x = cx + d * 16, a1y = cy - 19 + w1;
  int a2x = cx + d * 12, a2y = cy - 21 + w2;
  tft.drawLine(cx + d * 7, cy - 6, a1x, a1y, C_GOLD);
  tft.drawLine(cx + d * 5, cy - 7, a2x, a2y, C_GOLD);
  // Antenna tips (tiny dots)
  tft.fillCircle(a1x, a1y, 1, C_GOLD);
  tft.fillCircle(a2x, a2y, 1, C_GOLD);

  // ── LEGS ──
  for (int i = 0; i < 4; i++) {
    int lx = cx - d * 3 + d * i * 3;
    int anim = ((f / 3 + i) % 2) * 2;
    tft.drawLine(lx, cy + 5, lx - d, cy + 9 + anim, C_DRED);
  }
}

// ════════════════════════════════════════════
//  STATUS INDICATOR
// ════════════════════════════════════════════

void drawStatus(const char* label, uint16_t color) {
  int y = CLOCK_Y + (CLOCK_SIZE * 8) + 52;
  tft.fillRect(0, y - 2, SW, 22, C_BG);
  tft.fillCircle(SW / 2 - 20, y + 5, 3, color);
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND, C_BG);
  tft.setCursor(SW / 2 - 12, y + 2);
  tft.print(label);
}

// ════════════════════════════════════════════
//  WiFi CONNECTION
// ════════════════════════════════════════════

#ifdef PLATFORM_WAVESHARE
void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
  drawStatus("connecting", C_GOLD);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < WIFI_TIMEOUT)) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    wifiConnected = false;
    Serial.println("[WiFi] Connection failed");
  }
}

// ════════════════════════════════════════════
//  NOTIFICATION POLLING (HTTP)
// ════════════════════════════════════════════

void pollNotifications() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    return;
  }

  HTTPClient http;
  String url = String("http://") + API_HOST + ":" + API_PORT + API_PATH;
  http.begin(url);
  http.setTimeout(3000);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    parseNotifications(payload);
  } else if (code > 0) {
    Serial.printf("[API] HTTP %d\n", code);
  } else {
    Serial.printf("[API] Error: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

void parseNotifications(const String& json) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[API] JSON parse error: %s\n", err.c_str());
    return;
  }

  JsonArray arr = doc["notifications"].as<JsonArray>();
  notifCount = 0;

  for (JsonObject obj : arr) {
    if (notifCount >= MAX_NOTIFS) break;
    Notification& n = notifs[notifCount];
    strlcpy(n.type,    obj["type"]  | "info",    sizeof(n.type));
    strlcpy(n.title,   obj["title"] | "",        sizeof(n.title));
    strlcpy(n.body,    obj["body"]  | "",        sizeof(n.body));
    strlcpy(n.timeStr, obj["time"]  | "",        sizeof(n.timeStr));
    n.active = true;
    notifCount++;
  }

  if (notifCount > 0 && !showingNotif) {
    currentNotif = 0;
    showNotification(currentNotif);
    L.mood = (strcmp(notifs[0].type, "urgent") == 0) ? URGENT : ALERT;
  }
}
#endif

// ════════════════════════════════════════════
//  SERIAL COMMAND INTERFACE
//  Send JSON via Serial for testing on any platform:
//  {"type":"email","title":"Chris Luce","body":"FY27 budget review"}
// ════════════════════════════════════════════

void checkSerial() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  Serial.printf("[Serial] Received: %s\n", line.c_str());

  // Simple JSON parse for notification
  if (line.startsWith("{")) {
    #ifdef PLATFORM_WAVESHARE
      parseNotifications(String("{\"notifications\":[") + line + "]}");
    #else
      // Minimal parse for Wokwi (no ArduinoJson)
      parseSerialNotif(line);
    #endif
  }
  else if (line == "dismiss") {
    dismissNotification();
  }
  else if (line == "status") {
    Serial.printf("[Status] WiFi=%s Notifs=%d Mood=%d\n",
      wifiConnected ? "yes" : "no", notifCount, L.mood);
  }
}

#ifndef PLATFORM_WAVESHARE
// Lightweight serial notification parser for Wokwi (no ArduinoJson)
void parseSerialNotif(const String& json) {
  Notification& n = notifs[0];
  n.active = true;
  notifCount = 1;

  // Extract type
  int ti = json.indexOf("\"type\"");
  if (ti >= 0) {
    int q1 = json.indexOf('\"', json.indexOf(':', ti) + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) json.substring(q1 + 1, q2).toCharArray(n.type, sizeof(n.type));
  } else {
    strcpy(n.type, "info");
  }

  // Extract title
  ti = json.indexOf("\"title\"");
  if (ti >= 0) {
    int q1 = json.indexOf('\"', json.indexOf(':', ti) + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) json.substring(q1 + 1, q2).toCharArray(n.title, sizeof(n.title));
  } else {
    strcpy(n.title, "Notification");
  }

  // Extract body
  ti = json.indexOf("\"body\"");
  if (ti >= 0) {
    int q1 = json.indexOf('\"', json.indexOf(':', ti) + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) json.substring(q1 + 1, q2).toCharArray(n.body, sizeof(n.body));
  } else {
    strcpy(n.body, "");
  }

  strcpy(n.timeStr, "");
  showNotification(0);
  L.mood = (strcmp(n.type, "urgent") == 0) ? URGENT : ALERT;
}
#endif

// ════════════════════════════════════════════
//  NOTIFICATION DISPLAY
// ════════════════════════════════════════════

void showNotification(int idx) {
  if (idx >= notifCount) return;
  Notification& n = notifs[idx];

  showingNotif = true;
  notifShowTime = millis();

  // Map notification type to scenario
  if (strcmp(n.type, "email") == 0)         currentScenario = SC_EMAIL;
  else if (strcmp(n.type, "teams") == 0)    currentScenario = SC_TEAMS_MSG;
  else if (strcmp(n.type, "calendar") == 0) currentScenario = SC_MEETING_SOON;
  else if (strcmp(n.type, "urgent") == 0)   currentScenario = SC_LATE;
  else                                      currentScenario = SC_NEEDS_INPUT;

  // Render scenario card (icon badge + short message)
  renderScenarioCard(currentScenario);

  // Notification card area (below scenario card, for detail text)
  int cardY = CLOCK_Y + (CLOCK_SIZE * 8) + 20;
  int cardH = 70;

  // Pick color based on type
  uint16_t accent = C_NOTIF;
  const char* icon = "i";
  if (strcmp(n.type, "email") == 0)    { accent = C_NOTIF; icon = "@"; }
  if (strcmp(n.type, "teams") == 0)    { accent = C_TEAMS; icon = "T"; }
  if (strcmp(n.type, "calendar") == 0) { accent = C_STATUS; icon = "C"; }
  if (strcmp(n.type, "urgent") == 0)   { accent = C_WARN;  icon = "!"; }

  // Card background
  tft.fillRoundRect(4, cardY, SW - 8, cardH, 4, 0x2104);
  // Accent bar on left
  tft.fillRect(4, cardY, 4, cardH, accent);

  // Icon badge
  tft.fillCircle(18, cardY + 14, 8, accent);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(15, cardY + 11);
  tft.print(icon);

  // Title
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(30, cardY + 8);
  // Truncate title for screen width
  char titleBuf[32];
  strncpy(titleBuf, n.title, min((int)sizeof(titleBuf) - 1, (SW - 40) / 6));
  titleBuf[min((int)sizeof(titleBuf) - 1, (SW - 40) / 6)] = '\0';
  tft.print(titleBuf);

  // Time (top right)
  if (strlen(n.timeStr) > 0) {
    tft.setTextColor(C_LGRAY);
    tft.setCursor(SW - 40, cardY + 8);
    tft.print(n.timeStr);
  }

  // Body (word-wrapped, 2 lines max)
  tft.setTextColor(C_GRAY);
  tft.setCursor(12, cardY + 26);
  int maxChars = (SW - 20) / 6;
  if ((int)strlen(n.body) <= maxChars) {
    tft.print(n.body);
  } else {
    // Line 1
    char line1[48];
    strncpy(line1, n.body, min((int)sizeof(line1) - 1, maxChars));
    line1[min((int)sizeof(line1) - 1, maxChars)] = '\0';
    tft.print(line1);
    // Line 2
    tft.setCursor(12, cardY + 38);
    char line2[48];
    strncpy(line2, n.body + maxChars, min((int)sizeof(line2) - 1, maxChars));
    line2[min((int)sizeof(line2) - 1, maxChars)] = '\0';
    tft.print(line2);
  }

  // Notification count indicator
  if (notifCount > 1) {
    tft.setTextColor(C_LGRAY);
    tft.setCursor(SW / 2 - 10, cardY + cardH - 12);
    char countBuf[10];
    snprintf(countBuf, sizeof(countBuf), "%d/%d", idx + 1, notifCount);
    tft.print(countBuf);
  }

  Serial.printf("[Notif] Showing: [%s] %s\n", n.type, n.title);
}

void dismissNotification() {
  if (!showingNotif) return;

  // Clear notification card area
  int cardY = CLOCK_Y + (CLOCK_SIZE * 8) + 20;
  tft.fillRect(0, cardY, SW, 80, C_BG);

  // Clear scenario card
  clearScenarioCard();
  currentScenario = SC_IDLE;

  // Restore status
  drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);

  showingNotif = false;
  L.mood = IDLE;

  // Show next notification if queued
  currentNotif++;
  if (currentNotif < notifCount) {
    showNotification(currentNotif);
    L.mood = ALERT;
  } else {
    notifCount = 0;
    currentNotif = 0;
  }
}
